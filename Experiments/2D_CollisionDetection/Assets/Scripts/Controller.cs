using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[RequireComponent(typeof(Camera))]
public class Controller : MonoBehaviour
{
    public float rotationSpeed = 10.0f;

    public VoxelBody body1;
    public VoxelBody body2;

    new Camera camera;
    void Awake()
    {
        camera = GetComponent<Camera>();
    }

    void Update()
    {
        Vector3 mousePos = camera.ScreenToWorldPoint(Input.mousePosition);
        mousePos.z = -1.0f;
        if (Input.GetMouseButton(0))
        {
            body2.transform.position = mousePos;
            body2.transform.rotation *= Quaternion.AngleAxis(-rotationSpeed * Input.GetAxis("Horizontal") * Time.deltaTime, Vector3.forward);
        }
        if (Input.GetButtonDown("Generate"))
        {
            body2.transform.position += (Vector3)VoxelBody.ComputePenetration(body2, body1);
        }
    }
}
