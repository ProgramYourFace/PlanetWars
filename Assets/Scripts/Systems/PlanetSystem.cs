using System;
using System.Runtime.InteropServices;
using Unity.Entities;
using UnityEngine;

[UpdateInGroup(typeof(InitializationSystemGroup))]
public class PlanetSystem : ComponentSystem
{
	IntPtr planetFormCS;
    IntPtr sphereFormCS;

    [StructLayout(LayoutKind.Sequential, Pack = 16)]
    struct PlanetFormArgs
    {
        public float _radius;
        public float _amplitude;
        public float _frequency;
    };

    [StructLayout(LayoutKind.Sequential, Pack = 16)]
    struct SphereFormArgs
    {
        public float _radius;
        public float _strength;
    };

    Entity CreatePlanet(Vector3 position, Vector3 bounds, Quaternion rotation, PlanetFormArgs args)
	{
        VoxelForm planetForm = new VoxelForm(position, bounds, rotation, planetFormCS, NativeUtility.CreateConstantBufferFromStruct(args));

        return World.GetExistingSystem<VoxelSystem>().CreateVoxelBody(planetForm);
    }

	protected override void OnCreateManager()
	{
        NativeSystem.OnInitialize += NativeInitialize;
		NativeSystem.OnDeinitialize += NativeDeinitialize;
	}

	void NativeInitialize()
	{
		byte[] byteCode = NativeUtility.LoadShaderBytes("PlanetForm");
		planetFormCS = NativeUtility.CreateComputeShader(byteCode, byteCode.Length);
        byteCode = NativeUtility.LoadShaderBytes("SphereForm");
        sphereFormCS = NativeUtility.CreateComputeShader(byteCode, byteCode.Length);
        

        Entity planet = CreatePlanet(Vector3.zero,
            Vector3.one * 5000.0f,
            Quaternion.Euler(10, 25, -25),
            new PlanetFormArgs() { _radius = 2500.0f,
                _amplitude = 1000.0f,
                _frequency = 500.0f });

        World.GetExistingSystem<VoxelSystem>().AddFormToBody(planet, new VoxelForm(Vector3.one * 1000.0f, Vector3.one * 2010.0f, Quaternion.Euler(35,23,250), sphereFormCS, NativeUtility.CreateConstantBufferFromStruct(new SphereFormArgs() { _radius = 1000.0f, _strength = -1.0f})));
        World.GetExistingSystem<VoxelSystem>().AddFormToBody(planet, new VoxelForm(Vector3.one * -1000.0f, Vector3.one * 2010.0f, Quaternion.Euler(35, 23, 21), sphereFormCS, NativeUtility.CreateConstantBufferFromStruct(new SphereFormArgs() { _radius = 1000.0f, _strength = -1.0f })));
        World.GetExistingSystem<VoxelSystem>().AddFormToBody(planet, new VoxelForm(Vector3.zero, Vector3.one * 2010.0f, Quaternion.Euler(35, 23, 250), sphereFormCS, NativeUtility.CreateConstantBufferFromStruct(new SphereFormArgs() { _radius = 1000.0f, _strength = -1.0f })));
    }

	void NativeDeinitialize()
	{
		NativeUtility.DeleteNativeResouce(ref planetFormCS);
        NativeUtility.DeleteNativeResouce(ref sphereFormCS);
    }

	protected override void OnUpdate()
	{

	}
}
