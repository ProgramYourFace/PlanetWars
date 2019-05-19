using System;
using Unity.Entities;
using UnityEngine;

[UpdateInGroup(typeof(InitializationSystemGroup))]
public class PlanetSystem : ComponentSystem
{
	IntPtr planetFormCS;
	//IntPtr imageFormCS;

	//public static IntPtr testVolume;
	/*
	void Initalize()
	{
		
		byte[] imageFormBC = NativeUtility.LoadShaderBytes("ImageForm");
		imageFormCS = NativeUtility.CreateComputeShader(imageFormBC, imageFormBC.Length);
		
		byte[] rawImage = Resources.Load<TextAsset>("RawVox/DemonMan").bytes;
		int width = BitConverter.ToInt32(rawImage, 4);
		int height = BitConverter.ToInt32(rawImage, 8);
		int depth = BitConverter.ToInt32(rawImage, 12);
		int bits = BitConverter.ToInt32(rawImage, 16);
		int stride = (bits / 8) * 4;
		int size = width * height * depth;
		byte[] pxData = new byte[size * stride];
		for (int i = 0; i < size; i++)
		{
			byte lower = rawImage[i * 2 + 20];
			byte upper = rawImage[i * 2 + 21];
			if (upper == 255)
				upper = 254;
			//R
			pxData[i * stride + 0] = 0;
			pxData[i * stride + 1] = 0;
			//G				    
			pxData[i * stride + 2] = 0;
			pxData[i * stride + 3] = 0;
			//B				    
			pxData[i * stride + 4] = 0;
			pxData[i * stride + 5] = 0;
			//A
			pxData[i * stride + 6] = lower;
			pxData[i * stride + 7] = upper;
		}
		testVolume = NativeUtility.CreateVolume(width, height, depth, pxData);

		Entity entity = EntityManager.CreateEntity(typeof(VoxelBody), typeof(VoxelForm));
		EntityManager.SetComponentData(entity,
			new VoxelForm()
			{
				bounds = new Vector3(width, height, depth),
				center = Vector3.zero,
				formArguments = IntPtr.Zero,
				formComputeShader = imageFormCS
			});
		EntityManager.SetComponentData(entity, new VoxelBody(Quaternion.identity));


		byte[] planetFormBC = NativeUtility.LoadShaderBytes("PlanetForm");
		planetFormCS = NativeUtility.CreateComputeShader(planetFormBC, planetFormBC.Length);
		
		CreateSphere(Vector3.right * 512, new Vector3(512, 512, 512), Quaternion.Euler(25, 5, 25));
		CreateSphere(Vector3.up * 512, new Vector3(512, 512, 512), Quaternion.Euler(10, 25, -25));
		CreateSphere(Vector3.forward * 512, new Vector3(512, 512, 512), Quaternion.Euler(60, 32, 0));
	}
	*/

	Entity CreateSphere(Vector3 position, Vector3 bounds, Quaternion rotation)
	{
		Entity entity = EntityManager.CreateEntity(typeof(VoxelBody), typeof(VoxelForm));
		EntityManager.SetComponentData(entity,
			new VoxelForm()
			{
				bounds = bounds,
				center = position,
				formArguments = IntPtr.Zero,
				formComputeShader = planetFormCS
			});

		EntityManager.SetComponentData(entity, new VoxelBody(rotation));
		return entity;
	}

	protected override void OnCreateManager()
	{
		NativeSystem.OnInitialize += NativeInitialize;
		NativeSystem.OnDeinitialize += NativeDeinitialize;
	}

	void NativeInitialize()
	{
		byte[] planetFormBC = NativeUtility.LoadShaderBytes("PlanetForm");
		planetFormCS = NativeUtility.CreateComputeShader(planetFormBC, planetFormBC.Length);

		//CreateSphere(Vector3.right * 256, new Vector3(512, 512, 512), Quaternion.Euler(25, 5, 25));
		CreateSphere(Vector3.zero, new Vector3(512, 512, 512), Quaternion.Euler(10, 25, -25));
		CreateSphere(Vector3.right * 512, new Vector3(512, 512, 512), Quaternion.Euler(25, 5, 25));
		CreateSphere(Vector3.up * 512, new Vector3(512, 512, 512), Quaternion.Euler(10, 25, -25));
		CreateSphere(Vector3.forward * 512, new Vector3(512, 512, 512), Quaternion.Euler(60, 32, 0));
	}

	void NativeDeinitialize()
	{
		NativeUtility.DeleteNativeResouce(ref planetFormCS);
	}

	protected override void OnUpdate()
	{

	}
}
