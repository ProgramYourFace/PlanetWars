#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define SAFE_RELEASE(s) if(s) s->Release(); s = NULL
#define SAFE_DELETE(s) if(s) delete s; s = nullptr
#define EXPORT extern "C" __declspec(dllexport)
#include <d3d11.h>
#include <d3dcompiler.h>
#include <comdef.h>
#include <string>
using namespace std;

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

#define DEBUG

class RenderEngine;

RenderEngine* GetRenderEngine();
void Log(const string& msg);

#define LOG(msg) Log(msg)

#define HCHK(call) { HRESULT hr = call; if(FAILED(hr)) LOG(std::string(_com_error(hr).ErrorMessage()) + " - Line: " + std::to_string(__LINE__) + " - File: " + std::string(__FILE__)); }