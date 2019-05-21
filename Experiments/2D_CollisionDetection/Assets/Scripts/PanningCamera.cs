using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[RequireComponent(typeof(Camera))]
public class PanningCamera : MonoBehaviour
{
    public int zoomInterval = 1;
    public int minZoom = 1;
    public int maxZoom = 100;

    new Camera camera;
    Vector2 mouseDragStart;
    bool panning = false;

    static PanningCamera instance;

    void Awake()
    {
        instance = this;
        camera = GetComponent<Camera>();
        camera.orthographic = true;
    }

    void Update()
    {
        float scroll = Input.mouseScrollDelta.y;
        bool scrolled = Mathf.Abs(scroll) > Mathf.Epsilon;

        if(scrolled)
        {
            mouseDragStart = ScreenToWorldPoint(Input.mousePosition);
            camera.orthographicSize = Mathf.Clamp(camera.orthographicSize - scroll * zoomInterval, minZoom, maxZoom);
        }
        
        Vector2 mouseWorldPos = ScreenToWorldPoint(Input.mousePosition);
        if (panning || scrolled)
        {
            transform.position += new Vector3(mouseDragStart.x - mouseWorldPos.x, mouseDragStart.y - mouseWorldPos.y, 0.0f);
        }

        if (Input.GetMouseButtonDown(2))
        {
            mouseDragStart = mouseWorldPos;
            Cursor.visible = false;
            panning = true;
        }
        else if (Input.GetMouseButtonUp(2))
        {
            panning = false;
            Cursor.visible = true;
        }
        else if (Input.GetButtonDown("Focus"))
        {
            transform.position = new Vector3(0,0, transform.position.z);
        }
    }

    public Vector2 ScreenToWorldPoint(Vector2 screenPoint)
    {
        return camera.ScreenToWorldPoint(screenPoint);
    }

    public static PanningCamera inst { get { return instance; } }
}