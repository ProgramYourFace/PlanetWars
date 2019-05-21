using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[RequireComponent(typeof(MeshRenderer))]
public class VoxelBody : MonoBehaviour
{
    public Vector2Int bodySize = new Vector2Int(15, 15);

    public Vector2Int previewIndex;
    [Range(0.0f,1.0f)]
    public float x = 0.5f;

    float[,] m_voxels;
    
    Texture2D m_voxelTexture;

    MeshRenderer m_renderer;
    MaterialPropertyBlock m_materialPropertyBlock;

    void Awake()
    {
        m_renderer = GetComponent<MeshRenderer>();
        m_materialPropertyBlock = new MaterialPropertyBlock();
        m_voxelTexture = new Texture2D(0, 0, TextureFormat.RFloat, false, true);
        m_voxelTexture.filterMode = FilterMode.Bilinear;
        RegenerateVoxels();
    }

#if UNITY_EDITOR
    void OnDrawGizmos()
    {
        if (!Application.isPlaying)
            return;

        Vector3 center = new Vector3(bodySize.x, bodySize.y, 0.0f) * 0.5f;
        Gizmos.color = Color.red;
        Gizmos.matrix = Matrix4x4.TRS(transform.position - Vector3.forward * 0.01f, transform.rotation, Vector3.one);
        Gizmos.DrawRay(-center + new Vector3(previewIndex.x + x, previewIndex.y), Vector3.up * Mathf.Clamp01(ComputeY(x)));
        Gizmos.color = Color.green;
        Gizmos.DrawRay(-center + new Vector3(previewIndex.x, previewIndex.y), Vector3.up);
        Gizmos.DrawRay(-center + new Vector3(previewIndex.x + 1, previewIndex.y), Vector3.up);
        Gizmos.DrawRay(-center + new Vector3(previewIndex.x, previewIndex.y), Vector3.right);
        Gizmos.DrawRay(-center + new Vector3(previewIndex.x, previewIndex.y + 1), Vector3.right);
    }

    float ComputeY(float x)
    {
        float v00 = m_voxels[previewIndex.x, previewIndex.y];
        float v10 = m_voxels[previewIndex.x + 1, previewIndex.y];
        float v01 = m_voxels[previewIndex.x, previewIndex.y + 1];
        float v11 = m_voxels[previewIndex.x + 1, previewIndex.y + 1];
        return (-v00 - (v10 - v00) * x) / (v01 + (v11 - v01) * x - v00 -(v10 - v00) * x);
    }
#endif

#if UNITY_EDITOR
    void OnValidate()
    {
        if (bodySize.x < 1)
            bodySize.x = 1;
        if (bodySize.y < 1)
            bodySize.y = 1;
        previewIndex.x = Mathf.Clamp(previewIndex.x, 0, bodySize.x - 1);
        previewIndex.y = Mathf.Clamp(previewIndex.y, 0, bodySize.y - 1);
        if (Application.isPlaying)
            RegenerateVoxels();
    }
#endif

    public virtual float SampleVoxel(Vector2 uv)
    {
        return -1.0f;
    }

    public void RegenerateVoxels()
    {
        Vector2 center = new Vector2(bodySize.x, bodySize.y) * 0.5f;
        Vector2Int size = bodySize + Vector2Int.one;
        float[,] v = new float[size.x, size.y];
        for (int x = 0; x < size.x; x++)
        {
            for (int y = 0; y < size.y; y++)
            {
                v[x, y] = SampleVoxel(new Vector2(x,y) - center);
            }
        }
        SetVoxels(v);
    }

    public void SetVoxels(float[,] newVoxels)
    {
        m_voxels = newVoxels;
        UpdateVisual();
    }

    public void UpdateVisual()
    {
        if (m_voxelTexture == null || m_materialPropertyBlock == null)
            return;
        int width = m_voxels.GetLength(0);
        int height = m_voxels.GetLength(1);

        transform.localScale = new Vector3(width - 1.0f, height - 1.0f, 1.0f);

        if (m_voxelTexture.width != width || m_voxelTexture.height != height)
            m_voxelTexture.Resize(width, height);
        
        Color[] newPixels = new Color[width * height];
        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                float v = m_voxels[x, y];
                newPixels[x + y * width] = new Color(v, v, v, v);
            }
        }
        m_voxelTexture.SetPixels(newPixels);
        m_voxelTexture.Apply();
        m_materialPropertyBlock.SetTexture("_Voxels", m_voxelTexture);
        m_renderer.SetPropertyBlock(m_materialPropertyBlock);
    }

    /// <summary>
    /// Compute displacement vector that if applied to vb1 would move it out of penetration with vb2
    /// </summary>
    /// <param name="vb1">First voxel body</param>
    /// <param name="vb2">Second voxel body</param>
    /// <returns>Depenetration vector</returns>
    public static Vector2 ComputePenetration(VoxelBody vb1, VoxelBody vb2)
    {
        return Vector2.one;
    }
}
