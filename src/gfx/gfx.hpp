#pragma once

#include <utils/int.hpp>

#include <gfx/dx11.hpp>

class Gfx
{
public:
	Gfx(HWND hWnd_);

	auto update() -> void;
	auto present() -> void;

	auto createVb(const void* data, UINT byteSize, UINT byteStride) -> ComPtr<ID3D11Buffer>;
	auto createIb(const void* data, UINT byteSize, UINT byteStride) -> ComPtr<ID3D11Buffer>;

	ComPtr<ID3D11Device> device;
	ComPtr<IDXGISwapChain> swapChain;
	ComPtr<ID3D11DeviceContext> ctx;
	ComPtr<ID3D11RenderTargetView> backBufferRenderTargetView;

	static constexpr UINT SWAP_CHAIN_BUFFER_COUNT{ 1 };
	static constexpr DXGI_FORMAT SWAP_CHAIN_FORMAT{ DXGI_FORMAT_B8G8R8A8_UNORM };
	static constexpr UINT SWAP_CHAIN_FLAGS{ 0 };

private:
	auto setBackBufferRenderTargetView() -> void;
};