#include "ChunkMeshingIncludes.hlsli"

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= _chunkSize.x || id.y >= _chunkSize.y || id.z >= _chunkSize.z)
        return;
    
	uint3 corners[8];
	half4 cube[8];
	uint cubeFlag = 0;
	uint i, j;
	[unroll(8)]for (i = 0; i < 8; i++)
	{
		corners[i] = id + corner[i];
		if ((cube[i] = _voxels.Load(int4(corners[i], 0))).a <= _level)
			cubeFlag |= 1 << i;
	}

	uint edgeFlag = edgeTable[cubeFlag];
	if (edgeFlag == 0)
		return;
    
	float3 vertList[12];
	uint2 e;
	[unroll(12)]for (i = 0; i < 12; i++)
	{
		if ((edgeFlag & (1 << i)) != 0)
		{
			e = edgeCorners[i];
            vertList[i] = vInterp(corners[e.x], corners[e.y], cube[e.x].a, cube[e.y].a);
        }
	}
	
    Triangle t;
    for (i = 0; triTable[cubeFlag][i] != -1; i += 3)
    {
        j = triTable[cubeFlag][i];
        e = edgeCorners[j];
        t.pos[0] = vertList[j];
        t.col = (cube[e.x].a < cube[e.y].a ? cube[e.x] : cube[e.y]);

        j = triTable[cubeFlag][i + 1];
        e = edgeCorners[j];
        t.pos[1] = vertList[j];
        t.col += (cube[e.x].a < cube[e.y].a ? cube[e.x] : cube[e.y]);
		
        j = triTable[cubeFlag][i + 2];
        e = edgeCorners[j];
        t.pos[2] = vertList[j];
        t.col += (cube[e.x].a < cube[e.y].a ? cube[e.x] : cube[e.y]);
		
        t.col /= 3.0f;
        t.nrm = normalize(cross(t.pos[1] - t.pos[0], t.pos[2] - t.pos[0]));
		
        _triangles.Append(t);
    }
}