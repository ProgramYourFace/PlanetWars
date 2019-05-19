using UnityEngine;

public static class VoxelUtility
{
    public static int ArrayIndex(Vector3Int index, int size)
    {
        return ArrayIndex(index.x,index.y,index.z,size);
    }

    public static int ArrayIndex(int x, int y, int z,int size)
    {
        return x + y * size + z * size * size;
    }

    public static bool InRange(Vector3Int index, int d)
    {
        if (index.x < 0 || index.x >= d || index.y < 0 || index.y >= d || index.z < 0 || index.z >= d)
            return false;
        return true;
    }

	public static Vector3 Scale(this Vector3Int v3i, float scale)
	{
		return new Vector3(v3i.x * scale, v3i.y * scale, v3i.z * scale);
	}

	public static Vector3 Scale(this Vector3Int v3i, Vector3 scale)
	{
		return new Vector3(v3i.x * scale.x, v3i.y * scale.y, v3i.z * scale.z);
	}

	public static Vector3 Divide(this Vector3 v3, Vector3 d)
	{
		return new Vector3(v3.x / d.x, v3.y / d.y, v3.z / d.z);
	}
}