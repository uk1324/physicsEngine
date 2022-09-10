#pragma once

#include <utils/int.hpp>

#include <gfx/dx11.hpp>

class Gfx
{
public:
	Gfx(HWND hWnd);

	void present();

	auto createVb(const void* data, UINT byteSize, UINT byteStride) -> ComPtr<ID3D11Buffer>;
	auto createIb(const void* data, UINT byteSize, UINT byteStride) -> ComPtr<ID3D11Buffer>;

	ComPtr<ID3D11Device> device;
	ComPtr<IDXGISwapChain> swapChain;
	ComPtr<ID3D11DeviceContext> ctx;
	ComPtr<ID3D11RenderTargetView> backBufferRenderTargetView;
};