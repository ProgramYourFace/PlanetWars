using System.Collections;
using System.Collections.Generic;
using UnityEngine;


namespace PAS
{
	public class SpaceCameraController : MonoBehaviour
	{
		public float sensitivity = 10.0f;
		public float rollSpeed = 10.0f;
		public float movementSpeed = 10.0f;
		public float shiftMult = 10.0f;
		public Vector2 rotation = Vector2.zero;

        bool wireframe = false;
        Quaternion orient = Quaternion.identity;

		void Awake()
		{
			Cursor.visible = false;
			Cursor.lockState = CursorLockMode.Locked;
		}

		void LateUpdate()
		{
            if (Input.GetKeyDown(KeyCode.Z))
            {
                wireframe = !wireframe;
                NativeUtility.SetWireframe(wireframe);
            }
            if (Input.GetKeyDown(KeyCode.Escape))
			{
				Application.Quit();
			}
			if (Input.GetKeyDown(KeyCode.C))
			{
				Cursor.visible = !Cursor.visible;
				Cursor.lockState = Cursor.visible ? CursorLockMode.None : CursorLockMode.Locked;
			}

			if (!Cursor.visible)
			{
				rotation += new Vector2(Input.GetAxis("Mouse X"), Input.GetAxis("Mouse Y")) * sensitivity;
				rotation.x %= 360;
				rotation.y = Mathf.Clamp(rotation.y, -89, 89);
				transform.rotation = orient * Quaternion.Euler(-rotation.y, rotation.x, 0f);
			}
			float roll = 0.0f;
			if (Input.GetKey(KeyCode.Q))
				roll = rollSpeed;
			if (Input.GetKey(KeyCode.E))
				roll -= rollSpeed;
			transform.rotation *= Quaternion.AngleAxis(roll * Time.fixedDeltaTime, Vector3.forward);

			Vector3 input = Vector3.ClampMagnitude(new Vector3(Input.GetAxisRaw("Horizontal"), 0.0f, Input.GetAxisRaw("Vertical")), 1.0f);
			if (Input.GetKey(KeyCode.Space))
				input.y = 1.0f;
			if (Input.GetKey(KeyCode.LeftControl))
				input.y -= 1.0f;

			transform.position += (transform.forward * input.z + transform.right * input.x + orient * Vector3.up * input.y) *
				movementSpeed * (Input.GetKey(KeyCode.LeftShift) ? shiftMult : 1.0f);



			if (input.sqrMagnitude > 0.000001f || Mathf.Abs(roll) > 0.00001f)
				ApplyToOrient();
		}

		void ApplyToOrient()
		{
			orient = transform.rotation;
			rotation = Vector2.zero;
		}
	}
}