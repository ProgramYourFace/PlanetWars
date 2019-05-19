#pragma once
#include "Resource.h"
#include <Eigen/Dense>

class Chunk;
class RenderEngine;
_declspec(align(16)) struct CameraShaderConstants;

//typedef bool(*BranchCondition) (Eigen::Vector3i corner, Eigen::Vector3i size);
//typedef void(*EvaluateChunk) (Chunk* chunk, bool update, Eigen::Vector3i resolution, Eigen::Vector3i corner, Eigen::Vector3i size);
typedef bool(*UpdateCheck) (Eigen::Vector3i corner, Eigen::Vector3i size);
typedef bool(*FormChunk) (Chunk* chunk, Eigen::Vector3i resolution, Eigen::Vector3i corner, Eigen::Vector3i size);
struct ID3D11Device;

class ChunkNTree : public Resource
{
public:
	ChunkNTree();
	void Traverse(RenderEngine* renderEngine,
		const Eigen::Vector3i& corner,
		const Eigen::Vector3i& size,
		const Eigen::Vector3i& focus,
		const Eigen::Matrix4f& model,
		const Eigen::Matrix4f& viewProjection,
		const float& error_threshold,
		const int& leaf_size,
		int& update_slots,
		FormChunk form);

	bool Raycast(
		Eigen::Matrix4f model,
		Eigen::Vector3f origin,
		Eigen::Vector3f direction,
		float& t);

	void DestroyBranches();
	void DestroyChunk();
	~ChunkNTree() override;

	static const int MAX_BRANCH_DIM = 4;

	Eigen::Vector3i corner;
	Eigen::Vector3i size;
	Eigen::Vector3i branchDim;
	ChunkNTree** branches;
	Chunk* chunk;

private:
	struct OrderBranch
	{
		ChunkNTree* branch;
		Eigen::Vector3i corner;
		Eigen::Vector3i size;
		float distance;
	};
	int orderedCount;
	OrderBranch* orderedBranches;
	
	bool InternalLocalRaycastVolume(
		Eigen::Vector3f origin,
		Eigen::Vector3f direction,
		const Eigen::Vector3i& step,
		float& t);
	bool InternalLocalRaycastBox(
		Eigen::Vector3f origin,
		Eigen::Vector3f direction,
		float& tmin, float& tmax);
	void DrawTraverse(RenderEngine* renderEngine,
		const Eigen::Vector3i& corner,
		const Eigen::Vector3i& size,
		const Eigen::Vector3i& focus,
		const Eigen::Matrix4f& model,
		const Eigen::Matrix4f& viewProjection);

	inline void DrawChunk(RenderEngine* renderEngine,
		const Eigen::Vector3i& corner,
		const Eigen::Vector3i& size,
		const Eigen::Matrix4f& model,
		const Eigen::Matrix4f& viewProjection);
	inline void InsertOrderBranch(const OrderBranch& branch, const int& start, const int& range);
	inline void PrimeBranches(const Eigen::Vector3i& target);
	inline float ComputeError(const Eigen::Vector3i& focus, const Eigen::Vector3i& corner, const Eigen::Vector3i& size);
	inline float SqrToBox(const Eigen::Vector3i& focus, const Eigen::Vector3i& corner, const Eigen::Vector3i& size);
};

