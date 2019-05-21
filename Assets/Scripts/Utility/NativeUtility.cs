using System;
using System.Runtime.InteropServices;
using UnityEngine;

[StructLayout(LayoutKind.Sequential, Pack = 16)]
public struct NativeSceneShaderConstants
{
	public Vector4 light;
};
[StructLayout(LayoutKind.Sequential, Pack = 16)]
public struct NativeCameraShaderConstants
{
	public Matrix4x4 viewProjection;
};
[StructLayout(LayoutKind.Sequential, Pack = 16)]
public struct NativeModelShaderConstants
{
	public Matrix4x4 localToWorld;
};
[StructLayout(LayoutKind.Explicit, Pack = 16, Size = 96)]
public struct BrushStrokeParameters
{
	[FieldOffset(0)]
	public Matrix4x4 voxelMatrix;
	[FieldOffset(64)]
	public Vector3Int viewStart;
	[FieldOffset(80)]
	public Vector3Int viewRange;
    [FieldOffset(76)]
    public Vector3 formBounds;
};

public enum NativeRenderTargetType
{
	Depth = 1,
	Color = 2
}

public static class NativeUtility
{
	public const string NATIVE_PW_DLL = "PlanetWars-Native";

	public delegate void LogCallback(string msg);
	//public delegate bool BranchCondition(Vector3Int corner, Vector3Int size);
	//public delegate void EvaluateChunk(IntPtr chunk, bool update, Vector3Int resolution, Vector3Int corner, Vector3Int size);
	public delegate void FormChunk(IntPtr chunk, Vector3Int resolution, Vector3Int corner, Vector3Int size);

	[DllImport(NATIVE_PW_DLL)]
	public static extern bool IsInitialized();
	[DllImport(NATIVE_PW_DLL)]
	public static extern void Initialize(LogCallback callback);
	[DllImport(NATIVE_PW_DLL)]
	public static extern void InitializeComputeResources(
		byte[] meshingCSByteCode,
		int meshingCSLength,
		byte[] skirtCSByteCode,
		int skirtCSLength,
		byte[] chunkVSByteCode,
		int chunkVSByteCodeLength,
		byte[] chunkPSByteCode,
		int chunkPSByteCodeLength);
	[DllImport(NATIVE_PW_DLL)]
	public static extern void InitializeMaxVertexBuffer(int maxResolution);
	[DllImport(NATIVE_PW_DLL)]
	public static extern void Deinitialize();
	
	/*[DllImport(NATIVE_PW_DLL)]
	public static extern void LockContext();
	[DllImport(NATIVE_PW_DLL)]
	public static extern void UnlockContext();*/
	[DllImport(NATIVE_PW_DLL)]
	public static extern IntPtr CreateComputeShader(byte[] byteCode, int byteCodeLength);
	[DllImport(NATIVE_PW_DLL)]
	public static extern void CopyNativeToUnityBuffer(IntPtr native, IntPtr unity);
	[DllImport(NATIVE_PW_DLL)]
	public static extern void CopyNativeToUnityVolume(IntPtr native, IntPtr unity);
	[DllImport(NATIVE_PW_DLL)]
	public static extern void DeleteNativeResouce(ref IntPtr resource);
	[DllImport(NATIVE_PW_DLL)]
	public static extern void ReleaseD3DResource(ref IntPtr resource);
	[DllImport(NATIVE_PW_DLL)]
	public static extern void ApplyForm(
		IntPtr computeShader,
		IntPtr voxelChunk,
		IntPtr arguments,
		BrushStrokeParameters brushParameters);
	[DllImport(NATIVE_PW_DLL)]
	public static extern void ComputeMesh(IntPtr voxelChunk, bool skirts);

	[DllImport(NATIVE_PW_DLL)]
	public static extern void SetSceneConstants(NativeSceneShaderConstants scene);
    [DllImport(NATIVE_PW_DLL)]
    public static extern void SetWireframe(bool wireframe);
    [DllImport(NATIVE_PW_DLL)]
	public static extern void Draw(IntPtr chunk, NativeModelShaderConstants modelConstants, Matrix4x4 viewProjection);
	[DllImport(NATIVE_PW_DLL)]
	public static extern void Render(IntPtr renderPackage);
	[DllImport(NATIVE_PW_DLL)]
	public static extern void Present();
	[DllImport(NATIVE_PW_DLL)]
	public static extern IntPtr GetRenderInjectFunc();

	[DllImport(NATIVE_PW_DLL)]
	private static extern IntPtr CreateConstantBuffer(IntPtr data, int byteCount);
	public static IntPtr CreateConstantBufferFromStruct<T>(T data) where T : struct
	{
		int size = Marshal.SizeOf<T>();
		IntPtr buffer = Marshal.AllocHGlobal(size);
		Marshal.StructureToPtr(data, buffer, false);
		IntPtr gpuBuffer = CreateConstantBuffer(buffer, size);
		Marshal.FreeHGlobal(buffer);
		return gpuBuffer;
	}

	public static byte[] LoadShaderBytes(string shaderName)
	{
		return Resources.Load<TextAsset>("NativeShaders/" + shaderName).bytes;
	}

    public static Matrix4x4 GetNativeViewProjection(this Camera camera)
    {
        return GL.GetGPUProjectionMatrix(camera.projectionMatrix, true) * camera.worldToCameraMatrix;
    }


	[DllImport(NATIVE_PW_DLL)]
	public static extern IntPtr CreateRenderTarget(int width, int height, bool hdr, bool depthOnly);
	[DllImport(NATIVE_PW_DLL)]
	public static extern IntPtr GetRenderPackage(IntPtr renderTarget, NativeCameraShaderConstants cameraConstants);

	//Volumes
	[DllImport(NATIVE_PW_DLL)]
	public static extern IntPtr CreateVolume(int width, int height, int depth, byte[] data);
	[DllImport(NATIVE_PW_DLL)]
	public static extern void CSBindVolume(int slot, IntPtr volume, bool write);

	//Chunk
	[DllImport(NATIVE_PW_DLL)]
	public static extern IntPtr CreateChunk(Vector3Int size);
	[DllImport(NATIVE_PW_DLL)]
	public static extern void LockChunk(IntPtr chunk);
	[DllImport(NATIVE_PW_DLL)]
	public static extern void UnlockChunk(IntPtr chunk);

	//ChunkNTree
	[DllImport(NATIVE_PW_DLL)]
	public static extern IntPtr CreateChunkNTree();
	[DllImport(NATIVE_PW_DLL)]
	public static extern int TraverseChunkNTree(IntPtr tree,
        Vector3Int corner,
        Vector3Int size,
        Vector3Int focus,
        Matrix4x4 model,
        Matrix4x4 viewProjection,
        float errorThreshold,
        int leafSize,
        int updateSlots,
        FormChunk form);
}
