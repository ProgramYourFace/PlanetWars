using Unity.Entities;
using Unity.Collections;
using UnityEngine;
using System;
using Unity.Collections.LowLevel.Unsafe;

public struct VoxelBody : IComponentData
{
	[NativeDisableUnsafePtrRestriction]
	public IntPtr tree;

    public void Initialize()
    {
        tree = NativeUtility.CreateChunkNTree();
    }
    
	public void Destroy()
	{
		NativeUtility.DeleteNativeResouce(ref tree);
	}
}

public struct VoxelForm : IComponentData
{
	public Vector3 center;
    public Vector3 bounds;
    public Quaternion rotation;
	public IntPtr formComputeShader;
	public IntPtr formArguments;

    public Entity next;
    public Entity last;

    int updateType;

    public VoxelForm(Vector3 center, Vector3 bounds, Quaternion rotation, IntPtr formComputeShader, IntPtr formArguments)
    {
        this.center = center;
        this.rotation = rotation;
        this.bounds = bounds;
        this.formComputeShader = formComputeShader;
        this.formArguments = formArguments;
        updateType = -1;
        next = Entity.Null;
        last = Entity.Null;
    }

    public void ReleaseArguments()
    {
        NativeUtility.DeleteNativeResouce(ref formArguments);
    }

    public void MarkDirty()
    {
        if (updateType == 0)
            updateType = 1;
    }

    public bool isNew { get { return updateType == -1; } }
    public bool isDirty { get { return updateType > 0; } }
}