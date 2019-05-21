#include "ChunkMeshingIncludes.hlsli"

static const uint facePos[6] =
{
    0,
    1,
    0,
    1,
    0,
    1,
};

static const uint2 faceAdj[6] =
{
    uint2(2, 1),
    uint2(1, 2),
    uint2(0, 2),
    uint2(2, 0),
    uint2(1, 0),
    uint2(0, 1),
};

static const int3 faceX[6] =
{
    int3( 0, 0, 1),
    int3( 0,-1, 0),
    int3( 1, 0, 0),
    int3( 0, 0,-1),
    int3( 0, 1, 0),
    int3(-1, 0, 0),
};

static const int3 faceY[6] =
{
    int3( 0, 1, 0),
    int3( 0, 0,-1),
    int3( 0, 0, 1),
    int3(-1, 0, 0),
    int3( 1, 0, 0),
    int3( 0,-1, 0),
};

static const float3 faceNrm[6] =
{
    float3(-1, 0, 0),
    float3( 1, 0, 0),
    float3(0, -1, 0),
    float3(0,  1, 0),
    float3(0, 0, -1),
    float3(0, 0,  1),
};

static const uint2 corners[4] = 
{
    uint2(0,0),
    uint2(1,0),
    uint2(1,1),
    uint2(0,1)
};

static const uint2 edges[4] =
{
    uint2(0, 1),
    uint2(1, 2),
    uint2(2, 3),
    uint2(3, 0),
};

static const uint squareEdgeTable[16] =
{
    0, 9, 3, 10, 6, 15, 5, 12, 12, 5, 15, 6, 12, 3, 9, 0
};

static const int squareTriTable[16][13] =
{
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0 }, //0
    { 4,  7,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 }, //1
    { 5,  4,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 }, //2
    { 5,  7,  0,  0,  1,  5, -1, -1, -1, -1, -1, -1, 2 }, //3
    { 6,  5,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 }, //4 -
    { 7,  0,  6,  0,  2,  6,  0,  5,  2,  4,  5,  0, 4 }, //5
    { 6,  4,  1,  1,  2,  6, -1, -1, -1, -1, -1, -1, 2 }, //6
    { 6,  7,  0,  0,  2,  6,  0,  1,  2, -1, -1, -1, 3 }, //7
    { 7,  6,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 }, //8
    { 6,  3,  4,  3,  0,  4, -1, -1, -1, -1, -1, -1, 2 }, //9
    { 1,  7,  4,  1,  3,  7,  1,  6,  3,  1,  5,  6, 4 }, //10
    { 6,  1,  5,  3,  1,  6,  3,  0,  1, -1, -1, -1, 3 }, //11
    { 7,  5,  2,  7,  2,  3, -1, -1, -1, -1, -1, -1, 2 }, //12
    { 2,  4,  5,  2,  0,  4,  2,  3,  0, -1, -1, -1, 3 }, //13
    { 3,  7,  4,  3,  4,  1,  3,  1,  2, -1, -1, -1, 3 }, //14
    { 0,  1,  2,  2,  3,  0, -1, -1, -1, -1, -1, -1, 2 }  //15
};
/*
static const int triNrmTable[16][8] =
{
    { -1, -1, -1, -1, -1, -1, -1, -1 }, //0
    {  4,  7, -1, -1, -1, -1, -1, -1 }, //1
    {  5,  4, -1, -1, -1, -1, -1, -1 }, //3
    {  5,  7,  5,  7, -1, -1, -1, -1 }, //4
    {  6,  7,  6,  7,  4,  5,  4,  5 }, //5
    {  6,  4,  6,  4, -1, -1, -1, -1 }, //6
    {  6,  7,  6,  7,  6,  7, -1, -1 }, //7
    {  7,  6, -1, -1, -1, -1, -1, -1 }, //8
    {  4,  6,  4,  6, -1, -1, -1, -1 }, //9
    {  7,  4,  7,  4,  5,  6,  5,  6 }, //10
    {  5,  6,  5,  6,  5,  6, -1, -1 }, //11
    {  7,  5,  7,  5, -1, -1, -1, -1 }, //12
    
    {  4,  5,  4,  5,  4,  5, -1, -1 }, //13
    {  7,  4,  7,  4,  7,  4, -1, -1 }, //14
    {  4,  7, -1, -1, -1, -1, -1, -1 }, //15
};*/

[numthreads(8, 8, 6)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint2 adjIdx = faceAdj[id.z];
    uint2 size = uint2(_chunkSize[adjIdx.x], _chunkSize[adjIdx.y]);

    if (id.x >= size.x || id.y >= size.y)
        return;

    int3 fX = faceX[id.z];
    int3 fY = faceY[id.z];
    int3 cellPos = _chunkSize * facePos[id.z] + id.x * fX + id.y * fY;

    int3 c;

    uint i;
    uint squareFlag = 0;
    uint2 e;
    half4 v;
    float3 points[8];
    float4 square[8];
    [unroll(4)]
    for (i = 0; i < 4;i++)
    {
        e = corners[i];
        c = cellPos + e.x * fX + e.y * fY;
        v = _voxels.Load(uint4(c, 0));
        points[i] = c;
        square[i] = v;
        if (v.a <= _level)
            squareFlag |= 1 << i;
    }
    
    if (squareFlag == 0 || squareFlag == 15)
        return;

	/*
    if(squareFlag == 15)
    {
        bool closed = true;
        c = cellPos - fX - fY;
        [unroll(3)]
        for (i = 0; i < 3;i++)
        {
            if (_voxels.Load(uint4(c, 0)).a > _level) closed = false;
            c += fX;
        }
        [unroll(3)]
        for (i = 0; i < 3; i++)
        {
            if (_voxels.Load(uint4(c, 0)).a > _level) closed = false;
            c += fY;
        }
        [unroll(3)]
        for (i = 0; i < 3; i++)
        {
            if (_voxels.Load(uint4(c, 0)).a > _level) closed = false;
            c -= fX;
        }
        [unroll(3)]
        for (i = 0; i < 3; i++)
        {
            if (_voxels.Load(uint4(c, 0)).a > _level) closed = false;
            c -= fY;
        }

        if(closed)
            return;
    }*/

    uint edgeFlag = squareEdgeTable[squareFlag];
    float4 c1;
    float4 c2;
    [unroll(4)]
    for (i = 0; i < 4;i++)
    {
        if ((edgeFlag & (1 << i)) != 0)
        {
            e = edges[i];
            c1 = square[e.x];
            c2 = square[e.y];
            points[4 + i] = vInterp(points[e.x], points[e.y], c1.a, c2.a);
            square[4 + i] = c1.a < c2.a ? c1 : c2;
        }
    }

    Triangle t;
    t.nrm = faceNrm[id.z];
    int triIndex[13] = squareTriTable[squareFlag];
    uint count = triIndex[12];
    int j = 0;
    for (i = 0; i < count; i++)
    {
        j = triIndex[i * 3];
        t.pos[0] = points[j];
        t.col = square[j];

        j = triIndex[i * 3 + 1];
        t.pos[1] = points[j];
        t.col += square[j];
        
        j = triIndex[i * 3 + 2];
        t.pos[2] = points[j];
        t.col += square[j];
		
        t.col /= 3.0f;
        //t.nrm = normalize(cross(t.pos[1] - t.pos[0], t.pos[2] - t.pos[0]));
		
        _triangles.Append(t);
    }
}