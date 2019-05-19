using Unity.Entities;
using Unity.Jobs;
using Unity.Collections;
using UnityEngine;
using System;
using Unity.Collections.LowLevel.Unsafe;
using UnityEngine.Rendering;

struct NativeRenderJob : IJob
{
	[NativeDisableUnsafePtrRestriction]
	public IntPtr renderPackage;

	public void Execute()
	{
		NativeUtility.Render(renderPackage);
	}
}

[UpdateInGroup(typeof(InitializationSystemGroup))]
public class NativeSystem : ComponentSystem
{
	Camera nativeCamera;
	IntPtr renderTarget;
	Vector2Int renderSize;
	CommandBuffer depthInjector;
	CommandBuffer colorInjector;

	public static Action OnInitialize;
	public static Action OnDeinitialize;

    private static JobHandle currentRenderJob;

	protected override void OnStartRunning()
	{
		NativeUtility.Initialize(NativeLogger);
		byte[] chunkMeshingCSBytes = NativeUtility.LoadShaderBytes("ChunkMeshing");
		byte[] chunkMeshingSkirtsCSBytes = NativeUtility.LoadShaderBytes("ChunkMeshingSkirts");
		byte[] chunkVSBytes = NativeUtility.LoadShaderBytes("ChunkVertexShader");
		byte[] chunkPSBytes = NativeUtility.LoadShaderBytes("ChunkPixelShader");
		NativeUtility.InitializeComputeResources(
			chunkMeshingCSBytes,
			chunkMeshingCSBytes.Length,
			chunkMeshingSkirtsCSBytes,
			chunkMeshingSkirtsCSBytes.Length,
			chunkVSBytes,
			chunkVSBytes.Length,
			chunkPSBytes,
			chunkPSBytes.Length);
		
		depthInjector = new CommandBuffer();
		depthInjector.name = "DepthInjector";
		colorInjector = new CommandBuffer();
		colorInjector.name = "ColorInjector";
		OnInitialize?.Invoke();
		OnInitialize = null;
	}

    protected override void OnDestroy()
    {
        Deinitialize();
    }

    protected override void OnUpdate()
	{
        if (NativeUtility.IsInitialized())
		{
			//Maintain correct camera reference and command buffer integration.
			if (Camera.main != nativeCamera)
			{
				if (nativeCamera)
				{
					nativeCamera.RemoveCommandBuffer(CameraEvent.BeforeDepthTexture, depthInjector);
					nativeCamera.RemoveCommandBuffer(CameraEvent.BeforeForwardOpaque, colorInjector);
				}

				nativeCamera = Camera.main;

				if (nativeCamera)
				{
					nativeCamera.AddCommandBuffer(CameraEvent.BeforeDepthTexture, depthInjector);
					nativeCamera.AddCommandBuffer(CameraEvent.BeforeForwardOpaque, colorInjector);
				}
			}

            //Render scene to current camera.
            if (nativeCamera)
			{
				Vector2Int realRenderSize = new Vector2Int(nativeCamera.pixelWidth, nativeCamera.pixelHeight);
				if (renderSize != realRenderSize)
				{
					renderSize = realRenderSize;
					UpdateRenderTarget();
				}

				if(!Input.GetKey(KeyCode.R))
					Render(nativeCamera, renderTarget);
			}
			NativeUtility.Present();
			JobHandle.ScheduleBatchedJobs();
		}
	}

    public static NativeCameraShaderConstants GetCameraConstants(Camera camera)
    {
        NativeCameraShaderConstants camConsts = new NativeCameraShaderConstants();
        camConsts.viewProjection = GL.GetGPUProjectionMatrix(camera.projectionMatrix, true) * camera.worldToCameraMatrix;
        return camConsts;
    }

	public static void Render(Camera camera, IntPtr renderTarget)
	{
		currentRenderJob = new NativeRenderJob() { renderPackage = NativeUtility.GetRenderPackage(renderTarget, GetCameraConstants(camera)) }.Schedule(currentRenderJob);
	}

    public void Deinitialize()
    {
        if (nativeCamera)
        {
            if (depthInjector != null)
                nativeCamera.RemoveCommandBuffer(CameraEvent.BeforeDepthTexture, depthInjector);
            if (colorInjector != null)
                nativeCamera.RemoveCommandBuffer(CameraEvent.BeforeForwardOpaque, colorInjector);
        }
        depthInjector?.Release();
        colorInjector?.Release();
        depthInjector = null;
        colorInjector = null;

        currentRenderJob.Complete();
        NativeUtility.DeleteNativeResouce(ref renderTarget);
        OnDeinitialize?.Invoke();
        OnDeinitialize = null;
        NativeUtility.Deinitialize();
    }

    void UpdateRenderTarget()
	{
		depthInjector.Clear();
		colorInjector.Clear();
		NativeUtility.DeleteNativeResouce(ref renderTarget);
		renderTarget = NativeUtility.CreateRenderTarget(renderSize.x, renderSize.y, nativeCamera.allowHDR, false);
		depthInjector.IssuePluginEventAndData(NativeUtility.GetRenderInjectFunc(), (int)(NativeRenderTargetType.Depth), renderTarget);
		colorInjector.IssuePluginEventAndData(NativeUtility.GetRenderInjectFunc(), (int)(NativeRenderTargetType.Depth | NativeRenderTargetType.Color), renderTarget);
	}

	private static void NativeLogger(string msg)
	{
		Debug.Log("<color=#0000ff>[Native]: " + msg + "</color>");
	}

    public Camera NativeCamera { get { return nativeCamera; } }
}