using Unity.Entities;
using Unity.Collections;
using UnityEngine;
using System;
using Unity.Collections.LowLevel.Unsafe;

public struct VoxelBody : IComponentData
{

	[NativeDisableUnsafePtrRestriction]
	public IntPtr tree;
	public Quaternion rotation;

	public VoxelBody(Quaternion rotation)
	{
		this.rotation = rotation;
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
	public IntPtr formComputeShader;
	public IntPtr formArguments;
}