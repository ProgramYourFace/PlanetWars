using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class RectangleVB : VoxelBody
{
    public Rect rect;
    public float rotate;

    public override float SampleVoxel(Vector2 uv)
    {
        Vector2 center = new Vector2(rect.x, rect.y);
        Vector2 extent = rect.size * 0.5f;

        uv -= new Vector2(rect.x, rect.y);

        Vector2 rotx = new Vector2(Mathf.Cos(rotate * Mathf.Deg2Rad), Mathf.Sin(rotate * Mathf.Deg2Rad));
        Vector2 roty = new Vector2(-rotx.y, rotx.x);
        uv = uv.x * rotx + uv.y * roty;

        Vector2 d = new Vector2(Mathf.Abs(uv.x), Mathf.Abs(uv.y)) - extent;
        return new Vector2(Mathf.Max(d.x, 0.0f), Mathf.Max(d.y, 0.0f)).magnitude + Mathf.Min(Mathf.Max(d.x, d.y), 0.0f);
    }
}