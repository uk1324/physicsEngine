#include <gfx/gfx.hpp>
#include <winUtils.hpp>

Gfx::Gfx(HWND hWnd) {
	auto init = [&] {
		DXGI_SWAP_CHAIN_DESC swapChainDesc{
			.BufferDesc = {
				.Width = 0,
				.Height = 0,
				.RefreshRate {
					.Numerator = 0,
					.Denominator = 0,
				},
				.Format = DXGI_FORMAT_B8G8R8A8_UNORM,
				.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
				.Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
			},
			.SampleDesc {
				// No anti-aliasing.
				.Count = 1,
				.Quality = 0,
			},
			.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
			.BufferCount = 1, // Do double buffering.
			.OutputWindow = hWnd,
			.Windowed = TRUE,
			.SwapEffect = DXGI_SWAP_EFFECT_DISCARD,
			.Flags = 0,
		};
		CHECK_WIN_HRESULT(D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			// Log additional errors to output
			// Can get strings using IDXGIInfoQueue 
			// e14
			D3D11_CREATE_DEVICE_DEBUG,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&swapChainDesc,
			swapChain.GetAddressOf(),
			device.GetAddressOf(),
			nullptr,
			ctx.GetAddressOf()
		));

		{
			ComPtr<ID3D11Resource> backBuffer;
			CHECK_WIN_HRESULT(swapChain->GetBuffer(
				0 /* 0 is the back buffer */,
				__uuidof(ID3D11Resource),
				reinterpret_cast<void**>(backBuffer.GetAddressOf())
			));
			CHECK_WIN_HRESULT(device->CreateRenderTargetView(backBuffer.Get(), nullptr, backBufferRenderTargetView.GetAddressOf()));
		}
	};

	init();
}

void Gfx::present() {
	if (const auto presentResult{ swapChain->Present(1, 0) }; presentResult == ERROR_DEVICE_REMOVED)
		CHECK_WIN_HRESULT(device->GetDeviceRemovedReason());
}

static auto createBufffer(ComPtr<ID3D11Device>& device, D3D11_BIND_FLAG type, const void* data, UINT byteSize, UINT byteStride) -> ComPtr<ID3D11Buffer> {
	const D3D11_BUFFER_DESC desc{
		.ByteWidth = byteSize,
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = static_cast<UINT>(type),
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
		.StructureByteStride = byteStride,
	};
	const D3D11_SUBRESOURCE_DATA dataDesc{
		.pSysMem = data
	};
	ComPtr<ID3D11Buffer> vb;
	CHECK_WIN_HRESULT(device->CreateBuffer(&desc, &dataDesc, vb.GetAddressOf()));
	return vb;
}

auto Gfx::createVb(const void* data, UINT byteSize, UINT byteStride) -> ComPtr<ID3D11Buffer> {
	return createBufffer(device, D3D11_BIND_VERTEX_BUFFER, data, byteSize, byteStride);
}

auto Gfx::createIb(const void* data, UINT byteSize, UINT byteStride) -> ComPtr<ID3D11Buffer> {
	return createBufffer(device, D3D11_BIND_INDEX_BUFFER, data, byteSize, byteStride);
}