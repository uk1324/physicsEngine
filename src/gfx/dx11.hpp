#pragma once

// It probably doesn't make sense to have a seperate file for including d3d11.h and windows.h because d3d11.h includes windows.h anyway so it won't be any faster.

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#include <d3dcompiler.h>
#pragma comment(lib, "D3DCompiler.lib")
#include <DirectXMath.h>

#include <wrl.h>
using Microsoft::WRL::ComPtr;