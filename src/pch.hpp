#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#include <d3dcompiler.h>
#pragma comment(lib, "D3DCompiler.lib")
#include <DirectXMath.h>

#include <wrl.h>
using Microsoft::WRL::ComPtr;