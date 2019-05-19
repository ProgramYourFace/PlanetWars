using System;
using Unity.Collections;
using Unity.Entities;
using Unity.Jobs;
using UnityEngine;

public struct VoxelLODUpdateJob : IJobForEach<VoxelBody, VoxelForm>
{
	public int updateSlots;
	public float errorThreshold;
	public Vector3 observerPosition;
    public Matrix4x4 viewProjection;

    public void Execute(ref VoxelBody body, ref VoxelForm form)
	{
		if (body.tree == IntPtr.Zero)
			return;
		Vector3Int bodyCellCount = Vector3Int.CeilToInt(form.bounds / VoxelSystem.CELL_SCALE);
		Vector3 startCorner = -bodyCellCount.Scale(VoxelSystem.CELL_SCALE) * 0.5f;
		
		VoxelForm baseForm = form;
		baseForm.center = Vector3.zero;
		NativeUtility.FormChunk formChunk = (IntPtr chunk, Vector3Int resolution, Vector3Int corner, Vector3Int size) =>
		{
			Vector3 min = startCorner + corner.Scale(VoxelSystem.CELL_SCALE);
			Vector3 scale = size.Scale(VoxelSystem.CELL_SCALE);
			//NativeUtility.CSBindVolume(0, PlanetSystem.testVolume, false);
			ApplyForm(chunk, baseForm, resolution, min, scale);
		};

		Matrix4x4 model = Matrix4x4.TRS(form.center + body.rotation * startCorner, body.rotation, Vector3.one * VoxelSystem.CELL_SCALE);
		Vector3 opos = model.inverse.MultiplyPoint3x4(observerPosition);
		//NativeUtility.TraverseChunkNTree(body.tree, Vector3Int.zero, bodyCellCount, branchCondition, evaluateChunk, VoxelSystem.MAX_CELL_RESOLUTION);
		int leftover = NativeUtility.TraverseChunkNTree(body.tree, Vector3Int.zero, bodyCellCount, Vector3Int.RoundToInt(opos), model, viewProjection, VoxelSystem.LOD_ERROR_THRESHOLD, VoxelSystem.MAX_CELL_RESOLUTION, updateSlots, formChunk);
	}


	static void ApplyForm(IntPtr chunk, VoxelForm form, Vector3Int resolution, Vector3 corner, Vector3 range)
	{
		BrushStrokeParameters brush = new BrushStrokeParameters();

		Vector3 formExtent = form.bounds / 2.0f;
		Vector3 formMin = form.center - formExtent;
		Vector3 formMax = form.center + formExtent;
		

		brush.viewStart = Vector3Int.FloorToInt(resolution.Scale(formMin - corner).Divide(range));
		brush.viewStart.Clamp(Vector3Int.zero, resolution);
		brush.viewRange = Vector3Int.CeilToInt(resolution.Scale(formMax - corner).Divide(range));
		brush.viewRange.Clamp(Vector3Int.zero, resolution);
		brush.viewRange -= brush.viewStart;

		if (brush.viewRange.x <= 0 || brush.viewRange.y <= 0 || brush.viewRange.z <= 0)
			return;

		Vector3Int cellCount = resolution - Vector3Int.one;
		Vector3 scale = range.Divide(formExtent).Divide(cellCount);
		Vector3 pos = cellCount.Scale(corner - form.center).Divide(range);
		pos.Scale(scale);

		//brush.voxelMatrix = Matrix4x4.TRS(-Vector3.one, Quaternion.identity, new Vector3(2.0f / cellCount.x, 2.0f / cellCount.y, 2.0f / cellCount.z));
		brush.voxelMatrix = Matrix4x4.TRS(pos, Quaternion.identity, scale);
		NativeUtility.ApplyForm(form.formComputeShader, chunk, form.formArguments, brush);
	}
}

[UpdateInGroup(typeof(InitializationSystemGroup))]
[UpdateBefore(typeof(NativeSystem))]
public class VoxelSystem : ComponentSystem
{
	public static int LOD_UPDATE_SLOTS = 10;
	public static float LOD_ERROR_THRESHOLD = .01f;
	public const int MAX_CELL_RESOLUTION = 31;
	public const float CELL_SCALE = 0.5f;

	struct DestroyVoxelBodies : IJobForEach<VoxelBody>
	{
		public void Execute(ref VoxelBody body)
		{
			body.Destroy();
		}
	}
    
    NativeSystem nativeSystem;

    protected override void OnCreateManager()
    {
        nativeSystem = World.GetExistingSystem<NativeSystem>();

        NativeSystem.OnInitialize += NativeInitialize;
		NativeSystem.OnDeinitialize += NativeDeinitialize;
	}

	void NativeInitialize()
	{
		NativeUtility.InitializeMaxVertexBuffer(MAX_CELL_RESOLUTION);
	}

    Matrix4x4 temp;
	protected override void OnUpdate()
	{
        Camera c = nativeSystem.NativeCamera ? nativeSystem.NativeCamera : Camera.main;
        if (!c)
            return;
        
        if (updatePosition)
            temp = NativeSystem.GetCameraConstants(c).viewProjection;

        Vector3 obsPos;
        if (GetObserverPosition(out obsPos))
            (new VoxelLODUpdateJob() { observerPosition = obsPos,
                errorThreshold = LOD_ERROR_THRESHOLD,
                updateSlots = LOD_UPDATE_SLOTS,
                viewProjection = temp
            }).Schedule(this).Complete();
	}
	
	void NativeDeinitialize()
	{
        EntityQuery eq = World.Active.EntityManager.CreateEntityQuery(typeof(VoxelBody));
        NativeArray<VoxelBody> vbArray = eq.ToComponentDataArray<VoxelBody>(Allocator.TempJob);
        for (int i = 0; i < vbArray.Length; i++)
            vbArray[i].Destroy();
        vbArray.Dispose();
    }

	bool updatePosition = true;
	Vector3 observerPosition;
	private bool GetObserverPosition(out Vector3 position)
	{
		if (Input.GetKeyDown(KeyCode.O))
			updatePosition = !updatePosition;

		if (!updatePosition)
		{
			position = observerPosition;
			return true;
		}
		if (Camera.main)
		{
			position = Camera.main.transform.position;
			observerPosition = position;
			return true;
		}

		position = Vector3.zero;
		return false;
	}
}