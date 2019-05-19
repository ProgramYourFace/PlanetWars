#include "Plugin.h"
#include "ChunkNTree.h"
#include "Chunk.h"
#include "RenderEngine.h"
#include <math.h>

using namespace Eigen;

#define DIM2COUNT(dim) (dim.x() * dim.y() * dim.z())
#define INT3EQUAL(a, b) (a.x() == b.x() && a.y() == b.y() && a.z() == b.z())
#define CLAMP(X, MIN, MAX) (X < MIN ? MIN : X > MAX ? MAX : X)

ChunkNTree::ChunkNTree() : branchDim(0,0,0)
{
	orderedBranches = nullptr;
	branches = nullptr;
	chunk = nullptr;
}

void ChunkNTree::Traverse(RenderEngine* renderEngine,
	const Vector3i& corner,
	const Vector3i& size,
	const Vector3i& focus,
	const Matrix4f& model,
	const Matrix4f& viewProjection,
	const float& error_threshold,
	const int& leaf_size,
	int& update_slots,
	FormChunk form)
{
	this->corner = corner;
	this->size = size;

	int minAxis = size.minCoeff();
	minAxis = max(minAxis, leaf_size);

	#define AXISCOUNT(axis) axis <= leaf_size ? 1 : (int)floor((float)axis / (float)minAxis) + 1
	

	Vector3i bDim(
		AXISCOUNT(size.x()),
		AXISCOUNT(size.y()),
		AXISCOUNT(size.z()));
	
	bDim[0] = min(bDim[0], MAX_BRANCH_DIM);
	bDim[1] = min(bDim[1], MAX_BRANCH_DIM);
	bDim[2] = min(bDim[2], MAX_BRANCH_DIM);
	
	if ((bDim.x() > 1 || bDim.y() > 1 || bDim.z() > 1) && ComputeError(focus, corner, size) > error_threshold)//Split
	{
		PrimeBranches(bDim);

		int stride = branchDim.x();
		int stridePitch = branchDim.x() * branchDim.y();
		
		int lastXs = 0;
		for (int x = 0; x < branchDim.x(); x++)
		{
			int newXs = ((x + 1) * size.x()) / branchDim.x();

			int lastYs = 0;
			for (int y = 0; y < branchDim.y(); y++)
			{
				int newYs = ((y + 1) * size.y()) / branchDim.y();

				int lastZs = 0;
				for (int z = 0; z < branchDim.z(); z++)
				{
					int newZs = ((z + 1) * size.z()) / branchDim.z();

					OrderBranch oBranch;
					oBranch.corner = Vector3i(corner.x() + lastXs, corner.y() + lastYs, corner.z() + lastZs);
					oBranch.size = Vector3i(newXs - lastXs, newYs - lastYs, newZs - lastZs);
					oBranch.distance = SqrToBox(focus, oBranch.corner, oBranch.size);
					oBranch.branch = branches[x + y * stride + z * stridePitch];
					InsertOrderBranch(oBranch, 0, orderedCount);

					lastZs = newZs;
				}

				lastYs = newYs;
			}
			lastXs = newXs;
		}

		for (int i = 0; i < orderedCount; i++)
		{
			OrderBranch& oBranch = orderedBranches[i];
			oBranch.branch->Traverse(renderEngine,
				oBranch.corner, 
				oBranch.size, 
				focus, 
				model,
				viewProjection,
				error_threshold, 
				leaf_size, 
				update_slots, 
				form);
		}
		if (update_slots >= 0)
			DestroyChunk();
		else
			DrawChunk(renderEngine, corner, size, model, viewProjection);
	}
	else//Leaf
	{
		Vector3i newRes;

		float maxAxis = (float)size.maxCoeff();
		if (maxAxis > leaf_size)
		{
			newRes = Vector3i(
				(int)round((float)(leaf_size * size.x()) / maxAxis) + 1,
				(int)round((float)(leaf_size * size.y()) / maxAxis) + 1,
				(int)round((float)(leaf_size * size.z()) / maxAxis) + 1);
		}
		else
		{
			newRes = Vector3i(size.x() + 1, size.y() + 1, size.z() + 1);
		}

		bool requestUpdate = (!chunk || chunk->size != newRes);
		if (requestUpdate)
		{
			if (update_slots > 0)
			{
				DestroyChunk();
				chunk = new Chunk(newRes, renderEngine->device);
				renderEngine->LockContext();
				form(chunk, newRes, corner, size);
				renderEngine->ComputeMesh(chunk);
				renderEngine->UnlockContext();
				requestUpdate = false;
			}
			update_slots--;
		}

		if (!requestUpdate)
			DestroyBranches();
		
		DrawTraverse(renderEngine, corner, size, focus, model, viewProjection);
	}
}

bool ChunkNTree::Raycast(Eigen::Matrix4f model, Eigen::Vector3f origin, Eigen::Vector3f direction, float & t)
{
	
	Matrix4f inv = model.inverse();
	Vector4f ori = inv * Vector4f(origin.x(), origin.y(), origin.z(), 1.0f);
	Vector4f dir = inv * Vector4f(direction.x(), direction.y(), direction.z(), 0.0f);
	
	float tmax;
	if (InternalLocalRaycastBox(ori.head<3>(), dir.head<3>(), t, tmax) && tmax > 0)
	{
		t = max(t, 0.0f);
		return true;
	}
	
	return false;
}

void ChunkNTree::DestroyBranches()
{
	if (branches)
	{
		int count = DIM2COUNT(branchDim);
		for (int i = 0; i < count; i++)
		{
			SAFE_DELETE(branches[i]);
		}
	}

	delete[] branches;
	branches = nullptr;
	branchDim = Vector3i(0,0,0);
}

void ChunkNTree::DestroyChunk()
{
	SAFE_DELETE(chunk);
}

ChunkNTree::~ChunkNTree()
{
	DestroyBranches();
	DestroyChunk();
}

bool ChunkNTree::InternalLocalRaycastVolume(Vector3f origin, Vector3f direction, const Eigen::Vector3i& step, float & t)
{
	if (branches)
	{
		Vector3i npos(	(int)((origin.x() - corner.x()) * branchDim.x() / size.x()),
						(int)((origin.y() - corner.y()) * branchDim.y() / size.y()),
						(int)((origin.z() - corner.z()) * branchDim.z() / size.z()));

		npos = {CLAMP(npos.x(), 0, branchDim.x()),
				CLAMP(npos.y(), 0, branchDim.y()),
				CLAMP(npos.z(), 0, branchDim.z())};

		int stride = branchDim.x();
		int stridePitch = branchDim.x() * branchDim.y();

		while (true)
		{
			ChunkNTree* branch = branches[npos.x() + npos.y() * stride + npos.z() * stridePitch];
			float branchT = t;
			if (branch->InternalLocalRaycastVolume(origin, direction, step, branchT))
			{
				t = branchT;
				return true;
			}
			else
			{
				#define E 0.000001f
				Vector3f hsize = { (float)branch->size.x() / 2.0f, (float)branch->size.y() / 2.0f, (float)branch->size.z() / 2.0f };
				float rtx = abs(direction.x()) < E ? HUGE_VALF : ((float)branch->corner.x() + hsize.x() + hsize.x() * step.x() - origin.x()) / direction.x();
				float rty = abs(direction.y()) < E ? HUGE_VALF : ((float)branch->corner.y() + hsize.y() + hsize.y() * step.y() - origin.y()) / direction.y();
				float rtz = abs(direction.z()) < E ? HUGE_VALF : ((float)branch->corner.z() + hsize.z() + hsize.z() * step.z() - origin.z()) / direction.z();

				if (rtx < rty)
				{
					if (rtx < rtz)
					{
						t += rtx;
						npos.x() += step.x();
						if (npos.x() < 0 || npos.x() >= branchDim.x())
							return false;
						origin += direction * rtx;
					}
					else
					{
						t += rtz;
						npos.z() += step.z();
						if (npos.z() < 0 || npos.z() >= branchDim.z())
							return false;
						origin += direction * rtz;
					}
				}
				else
				{
					if (rty < rtz)
					{
						t += rty;
						npos.y() += step.y();
						if (npos.y() < 0 || npos.y() >= branchDim.y())
							return false;
						origin += direction * rty;
					}
					else
					{
						t += rtz;
						npos.z() += step.z();
						if (npos.z() < 0 || npos.z() >= branchDim.z())
							return false;
						origin += direction * rtz;
					}
				}
			}
		}
	}
	else if(chunk)
	{
		return chunk->vertexCount != 0;
	}
	return true;
}

bool ChunkNTree::InternalLocalRaycastBox(Eigen::Vector3f origin, Eigen::Vector3f direction, float& tmin, float& tmax)
{
	int sx = direction.x() < 0;
	int sy = direction.y() < 0;
	int sz = direction.z() < 0;
	float rx = 1 / direction.x();
	float ry = 1 / direction.y();
	float rz = 1 / direction.z();
	origin.x() -= corner.x();
	origin.y() -= corner.y();
	origin.z() -= corner.z();

	float tymin, tymax, tzmin, tzmax;
	tmin = (sx * size.x() - origin.x()) * rx;
	tmax = ((1 - sx) * size.x() - origin.x()) * rx;
	tymin = (sy * size.y() - origin.y()) * ry;
	tymax = ((1 - sy) * size.y() - origin.y()) * ry;

	if ((tmin > tymax) || (tymin > tmax))
		return false;
	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;

	tzmin = (sz * size.z() - origin.z()) * rz;
	tzmax = ((1 - sz) * size.z() - origin.z()) * rz;

	if ((tmin > tzmax) || (tzmin > tmax))
		return false;
	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;

	return true;
}

void ChunkNTree::DrawTraverse(RenderEngine * renderEngine,
	const Eigen::Vector3i & corner,
	const Eigen::Vector3i & size,
	const Eigen::Vector3i& focus,
	const Eigen::Matrix4f & model,
	const Eigen::Matrix4f& viewProjection)
{
	DrawChunk(renderEngine, corner, size, model, viewProjection);

	if (branchDim.x() <= 0 || branchDim.y() <= 0 || branchDim.z() <= 0)
		return;

	PrimeBranches(branchDim);

	int stride = branchDim.x();
	int stridePitch = branchDim.x() * branchDim.y();

	int lastXs = 0;
	for (int x = 0; x < branchDim.x(); x++)
	{
		int newXs = ((x + 1) * size.x()) / branchDim.x();

		int lastYs = 0;
		for (int y = 0; y < branchDim.y(); y++)
		{
			int newYs = ((y + 1) * size.y()) / branchDim.y();

			int lastZs = 0;
			for (int z = 0; z < branchDim.z(); z++)
			{
				int newZs = ((z + 1) * size.z()) / branchDim.z();

				OrderBranch oBranch;
				oBranch.corner = Vector3i(corner.x() + lastXs, corner.y() + lastYs, corner.z() + lastZs);
				oBranch.size = Vector3i(newXs - lastXs, newYs - lastYs, newZs - lastZs);
				oBranch.distance = SqrToBox(focus, oBranch.corner, oBranch.size);
				oBranch.branch = branches[x + y * stride + z * stridePitch];
				InsertOrderBranch(oBranch, 0, orderedCount);

				lastZs = newZs;
			}
			lastYs = newYs;
		}
		lastXs = newXs;
	}

	for (int i = 0; i < orderedCount; i++)
	{
		OrderBranch& oBranch = orderedBranches[i];
		oBranch.branch->DrawTraverse(renderEngine,
			oBranch.corner,
			oBranch.size,
			focus,
			model,
			viewProjection);
	}
}

inline void ChunkNTree::DrawChunk(RenderEngine* renderEngine,
	const Eigen::Vector3i& corner,
	const Eigen::Vector3i& size,
	const Eigen::Matrix4f& model,
	const Eigen::Matrix4f& viewProjection
	)
{
	if (chunk)
	{
		ModelShaderConstants modelConsts;
		Transform<float, 3, Affine> transform(Translation3f((float)corner.x(), (float)corner.y(), (float)corner.z()) * Scaling(
			(float)size.x() / (float)(chunk->size.x() - 1.0f),
			(float)size.y() / (float)(chunk->size.y() - 1.0f),
			(float)size.z() / (float)(chunk->size.z() - 1.0f)));

		modelConsts.localToWorld = model;
		modelConsts.localToWorld *= transform.matrix();
		renderEngine->Draw(chunk, modelConsts, viewProjection);
	}
}

inline void ChunkNTree::InsertOrderBranch(const OrderBranch & branch, const int& start, const int& count)
{
	if (count <= 1)
	{
		int index = start;
		if (count == 1 && branch.distance >= orderedBranches[start].distance)
			index += 1;

		memmove(orderedBranches+(index+1), orderedBranches+index, (orderedCount-index)*sizeof(OrderBranch));
		orderedBranches[index] = branch;
		orderedCount += 1;
	}
	else
	{
		int hCount = count / 2;
		int hStart = start + hCount;
		if (branch.distance > orderedBranches[hStart].distance)
			InsertOrderBranch(branch, hStart, count - hCount);
		else
			InsertOrderBranch(branch, start, hCount);
	}
}

inline void ChunkNTree::PrimeBranches(const Eigen::Vector3i & target)
{
	int newCount = DIM2COUNT(target);
	if (target != branchDim)
	{
		ChunkNTree** newBranches = new ChunkNTree*[newCount];
		OrderBranch* newOrder = new OrderBranch[newCount];

		int oldCount = DIM2COUNT(branchDim);
		int maxCount = max(newCount, oldCount);
		int minCount = min(newCount, oldCount);
		if (minCount > 0)
			memcpy(newBranches, branches, minCount * sizeof(ChunkNTree*));

		for (int i = minCount; i < maxCount; i++)
		{
			if (i < newCount)//Allocate new chunks
			{
				newBranches[i] = new ChunkNTree();
			}
			else//Destroy old
			{
				delete branches[i];
			}
		}
		delete[] branches;
		delete[] orderedBranches;
		branches = newBranches;
		orderedBranches = newOrder;
		branchDim = target;
	}
	ZeroMemory(orderedBranches, sizeof(OrderBranch) * newCount);
	orderedCount = 0;
}

inline float ChunkNTree::ComputeError(const Eigen::Vector3i & focus, const Eigen::Vector3i& corner, const Eigen::Vector3i& size)
{
	float sqrDist = SqrToBox(focus, corner, size);
	return size.squaredNorm() / max(sqrDist * sqrDist, 0.00001f);
}

inline float ChunkNTree::SqrToBox(const Eigen::Vector3i & focus, const Eigen::Vector3i & corner, const Eigen::Vector3i & size)
{
	Vector3i farCorner = corner + size;

	Vector3i nearest(CLAMP(focus.x(), corner.x(), farCorner.x()),
		CLAMP(focus.y(), corner.y(), farCorner.y()),
		CLAMP(focus.z(), corner.z(), farCorner.z()));

	return (float)(nearest - focus).squaredNorm();
}

EXPORT ChunkNTree* CreateChunkNTree()
{
	return new ChunkNTree();
}

EXPORT int TraverseChunkNTree(ChunkNTree* tree,
	Vector3i corner,
	Vector3i size,
	Vector3i focus,
	Matrix4f model,
	Matrix4f viewProjection,
	float error_threshold,
	int leaf_size,
	int update_slots,
	FormChunk form)
{
	tree->Traverse(GetRenderEngine(), corner, size, focus, model, viewProjection, error_threshold, leaf_size, update_slots, form);
	return update_slots;
}
