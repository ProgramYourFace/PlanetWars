#include "Plugin.h"
#include "RenderEngine.h"
#include "IUnityInterface.h"
#include "IUnityGraphics.h"
#include "IUnityRenderingExtensions.h"

static IUnityInterfaces* s_UnityInterfaces = NULL;
static IUnityGraphics* s_Graphics = NULL;
static RenderEngine* s_RenderEngine = NULL;

typedef void(*logMSG_t) (const char* msg);
logMSG_t logMsg;
void Log(const string& msg) { if(logMsg) (*logMsg)(msg.c_str()); }
RenderEngine* GetRenderEngine() { return s_RenderEngine; }

EXPORT bool IsInitialized()
{
	return s_RenderEngine;
}

EXPORT void Initialize(logMSG_t logCallback)
{
	logMsg = logCallback;
	s_RenderEngine = new RenderEngine(s_UnityInterfaces);
	LOG("Native Initialized!");
}

EXPORT void InitializeComputeResources(
	void* meshingCSByteCode,
	int meshingCSLength,
	void* skirtCSByteCode,
	int skirtCSLength,
	void* chunkVSByteCode,
	int chunkVSByteCodeLength,
	void* chunkPSByteCode,
	int chunkPSByteCodeLength)
{
	s_RenderEngine->InitializeComputeResources(
		meshingCSByteCode,
		meshingCSLength,
		skirtCSByteCode,
		skirtCSLength,
		chunkVSByteCode,
		chunkVSByteCodeLength,
		chunkPSByteCode,
		chunkPSByteCodeLength);
}

EXPORT void Deinitialize()
{
	SAFE_DELETE(s_RenderEngine);
	LOG("Native Deinitialized!");
	logMsg = nullptr;
}

EXPORT void ReleaseD3DResource(IUnknown*& resource)
{
	SAFE_RELEASE(resource);
}

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
	switch (eventType)
	{
		case kUnityGfxDeviceEventInitialize:
		{
			break;
		}
		case kUnityGfxDeviceEventShutdown:
		{
			break;
		}
		case kUnityGfxDeviceEventBeforeReset:
		{
			break;
		}
		case kUnityGfxDeviceEventAfterReset:
		{
			break;
		}
	};
}

// Unity plugin load event
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
	s_UnityInterfaces = unityInterfaces;
	s_Graphics = unityInterfaces->Get<IUnityGraphics>();
	s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);
	OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

// Unity plugin unload event
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
	s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}

static void UNITY_INTERFACE_API OnRenderInject(int eventId, void* data)
{
	if (s_RenderEngine)
		s_RenderEngine->InjectRenderTarget(eventId, (RenderTarget*)data);
}

extern "C" UnityRenderingEventAndData UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderInjectFunc()
{
	return OnRenderInject;
}