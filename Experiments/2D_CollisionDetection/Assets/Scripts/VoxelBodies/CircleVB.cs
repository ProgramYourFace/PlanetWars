using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CircleVB : VoxelBody
{
    public Vector2 center;
    public float radius = 5.0f;

    public override float SampleVoxel(Vector2 uv)
    {
        return (uv - center).magnitude - radius;
    }
}