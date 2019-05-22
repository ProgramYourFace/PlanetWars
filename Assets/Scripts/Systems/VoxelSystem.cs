using System;
using System.Collections.Generic;
using Unity.Collections;
using Unity.Entities;
using Unity.Jobs;
using Unity.Burst;
using UnityEngine;

public struct VoxelLODUpdateJob : IJobForEachWithEntity<VoxelBody, VoxelForm>
{
    [ReadOnly] public int updateSlots;
    [ReadOnly] public float errorThreshold;
    [ReadOnly] public Vector3 observerPosition;
    [ReadOnly] public Matrix4x4 viewProjection;

    [ReadOnly] public ComponentDataFromEntity<VoxelForm> voxelForms;

    public void Execute(Entity entity, int index, [ReadOnly] ref VoxelBody body,[ReadOnly] ref VoxelForm form)
	{
        if (body.tree == IntPtr.Zero)
			return;
		Vector3Int bodyCellCount = Vector3Int.CeilToInt(form.bounds / VoxelSystem.CELL_SCALE);
		Vector3 startCorner = -bodyCellCount.Scale(VoxelSystem.CELL_SCALE) * 0.5f;

		VoxelForm cForm = form;
        cForm.center = Vector3.zero;
        cForm.rotation = Quaternion.identity;
        
        List<VoxelForm> forms = new List<VoxelForm>(1);
        
        while (true)
        {
            forms.Add(cForm);
            if (cForm.next != entity)
                cForm = voxelForms[cForm.next];
            else
                break;
        }

        NativeUtility.FormChunk formChunk = (IntPtr chunk, Vector3Int resolution, Vector3Int corner, Vector3Int size) =>
		{
			Vector3 min = startCorner + corner.Scale(VoxelSystem.CELL_SCALE);
			Vector3 scale = size.Scale(VoxelSystem.CELL_SCALE);
            for (int i = 0; i < forms.Count; i++)
                 ApplyForm(chunk, forms[i], resolution, min, scale);
        };

		Matrix4x4 model = Matrix4x4.TRS(form.center + form.rotation * startCorner, form.rotation, Vector3.one * VoxelSystem.CELL_SCALE);
		Vector3 opos = model.inverse.MultiplyPoint3x4(observerPosition);
		int leftover = NativeUtility.TraverseChunkNTree(body.tree, 
            Vector3Int.zero, 
            bodyCellCount, 
            Vector3Int.RoundToInt(opos), 
            model, 
            viewProjection, 
            VoxelSystem.LOD_ERROR_THRESHOLD, 
            VoxelSystem.MAX_CELL_RESOLUTION, 
            updateSlots, 
            formChunk);
	}


	static void ApplyForm(IntPtr chunk, VoxelForm form, Vector3Int resolution, Vector3 corner, Vector3 range)
	{
		BrushStrokeParameters brush = new BrushStrokeParameters();
        brush.formBounds = form.bounds;

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

        Quaternion iRot = Quaternion.Inverse(form.rotation);

		Vector3Int cellCount = resolution - Vector3Int.one;
		Vector3 scale = range.Divide(cellCount);
		Vector3 pos = iRot * (corner - form.center);
        
		brush.voxelMatrix = Matrix4x4.TRS(pos, iRot, scale);
		NativeUtility.ApplyForm(form.formComputeShader, chunk, form.formArguments, brush);
	}
}

[UpdateInGroup(typeof(InitializationSystemGroup))]
[UpdateBefore(typeof(NativeSystem))]
public class VoxelSystem : JobComponentSystem
{
	public static int LOD_UPDATE_SLOTS = 10;
	public static float LOD_ERROR_THRESHOLD = 7.5f;
	public const int MAX_CELL_RESOLUTION = 31;
	public const float CELL_SCALE = 1f;

	struct DestroyVoxelBodies : IJobForEach<VoxelBody>
	{
		public void Execute(ref VoxelBody body)
		{
			body.Destroy();
		}
	}

    Vector3 observerPosition;
    Matrix4x4 observerMat;

    EntityArchetype voxelBodyArchetype;
    EntityArchetype voxelFormArchetype;

    NativeSystem nativeSystem;

    protected override void OnCreateManager()
    {
        nativeSystem = World.GetExistingSystem<NativeSystem>();
        voxelBodyArchetype = EntityManager.CreateArchetype(typeof(VoxelBody), typeof(VoxelForm));
        voxelFormArchetype = EntityManager.CreateArchetype(typeof(VoxelForm));

        NativeSystem.OnInitialize += NativeInitialize;
		NativeSystem.OnDeinitialize += NativeDeinitialize;
	}

	void NativeInitialize()
	{
		NativeUtility.InitializeMaxVertexBuffer(MAX_CELL_RESOLUTION);
	}
    
    protected override JobHandle OnUpdate(JobHandle inputDeps)
    {
        if (GatherObserver())
        {
            inputDeps = (new VoxelLODUpdateJob()
            {
                observerPosition = observerPosition,
                errorThreshold = LOD_ERROR_THRESHOLD,
                updateSlots = LOD_UPDATE_SLOTS,
                viewProjection = observerMat,
                voxelForms = GetComponentDataFromEntity<VoxelForm>(true)
            }).Schedule(this, inputDeps);
        }
        return inputDeps;
    }
	
	void NativeDeinitialize()
	{
        EntityQuery eq = World.Active.EntityManager.CreateEntityQuery(typeof(VoxelBody));
        NativeArray<VoxelBody> vbArray = eq.ToComponentDataArray<VoxelBody>(Allocator.TempJob);
        for (int i = 0; i < vbArray.Length; i++)
            vbArray[i].Destroy();
        vbArray.Dispose();
        eq.Dispose();

        eq = World.Active.EntityManager.CreateEntityQuery(typeof(VoxelForm));
        NativeArray<VoxelForm> vfArray = eq.ToComponentDataArray<VoxelForm>(Allocator.TempJob);
        for (int i = 0; i < vfArray.Length; i++)
            vfArray[i].ReleaseArguments();
        vfArray.Dispose();
        eq.Dispose();
    }

    public Entity CreateVoxelBody(VoxelForm bodyBase)
    {
        VoxelBody vb = new VoxelBody();
        vb.Initialize();

        Entity entity = EntityManager.CreateEntity(voxelBodyArchetype);
        EntityManager.SetComponentData(entity, vb);
        bodyBase.last = entity;
        bodyBase.next = entity;
        EntityManager.SetComponentData(entity, bodyBase);
        return entity;
    }

    public void DestroyVoxelBody(Entity body)
    {
        VoxelBody vb = EntityManager.GetComponentData<VoxelBody>(body);
        vb.Destroy();
        
        Entity e = body;
        while (true)
        {
            VoxelForm vf = EntityManager.GetComponentData<VoxelForm>(e);
            vf.ReleaseArguments();

            if (e != body)
                EntityManager.DestroyEntity(e);

            if (vf.next != body)
                e = vf.next;
            else
                break;
        }
        EntityManager.DestroyEntity(body);
    }

    public Entity AddFormToBody(Entity body, VoxelForm form)
    {
        VoxelForm vfSource = EntityManager.GetComponentData<VoxelForm>(body);

        Entity last = vfSource.last == Entity.Null ? body : vfSource.last;

        Entity newEntity = EntityManager.CreateEntity(voxelFormArchetype);
        form.last = last;
        form.next = body;
        EntityManager.SetComponentData(newEntity, form);

        vfSource.last = newEntity;
        EntityManager.SetComponentData(body, vfSource);

        VoxelForm lForm = EntityManager.GetComponentData<VoxelForm>(last);
        lForm.next = newEntity;
        EntityManager.SetComponentData(last, lForm);

        return newEntity;
    }

    public void RemoveForm(Entity formEntity)
    {
        VoxelForm form = EntityManager.GetComponentData<VoxelForm>(formEntity);

        VoxelForm lForm = EntityManager.GetComponentData<VoxelForm>(form.last);
        lForm.next = form.next;
        EntityManager.SetComponentData(form.last, lForm);

        VoxelForm nForm = EntityManager.GetComponentData<VoxelForm>(form.next);
        nForm.last = form.last;
        EntityManager.SetComponentData(form.next, nForm);

        form.ReleaseArguments();
        EntityManager.DestroyEntity(formEntity);
    }

	bool updateObserver = true;
    private bool GatherObserver()
	{
		if (Input.GetKeyDown(KeyCode.O))
            updateObserver = !updateObserver;

		if (!updateObserver)
            return true;

        Camera c = nativeSystem.NativeCamera ? nativeSystem.NativeCamera : Camera.main;
        if (c)
		{
			observerPosition = c.transform.position;
            observerMat = c.GetNativeViewProjection();
            return true;
		}
        return false;
	}
}