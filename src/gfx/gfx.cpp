#include <gfx/gfx.hpp>
#include <winUtils.hpp>

#pragma comment(lib, "dxgi.lib")

Gfx::Gfx(HWND hWnd) {
	auto init = [&] {

		CHECK_WIN_HRESULT(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION, device.GetAddressOf(), nullptr, ctx.GetAddressOf()));

		static constexpr DXGI_FORMAT BUFFER_FORMAT{ DXGI_FORMAT_B8G8R8A8_UNORM };

		DXGI_SAMPLE_DESC sample{ .Count = 1, .Quality = 0 };
		for (UINT i = sample.Count; i <= 8; i++) {
			UINT quality;
			CHECK_WIN_HRESULT(device->CheckMultisampleQualityLevels(BUFFER_FORMAT, i, &quality));
			if (quality > 0) {
				sample.Count = i;
				sample.Quality = quality - 1;
			}
		}

		// TODO: Maybe add log about what MSAA level was chosen.

		DXGI_SWAP_CHAIN_DESC swapChainDesc{
			.BufferDesc = {
				.Width = 0,
				.Height = 0,
				.RefreshRate {
					.Numerator = 0,
					.Denominator = 0,
				},
				.Format = BUFFER_FORMAT,
				.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
				.Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
			},
			.SampleDesc = sample,
			.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
			.BufferCount = 1, // Do double buffering.
			.OutputWindow = hWnd,
			.Windowed = TRUE,
			.SwapEffect = DXGI_SWAP_EFFECT_DISCARD,
			.Flags = 0,
		};
		ComPtr<IDXGIFactory> factory;
		CHECK_WIN_HRESULT(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(factory.GetAddressOf())));
		// TODO: Using CreateSwapChain is not recommended look at documentation. Maybe change later.
		CHECK_WIN_HRESULT(factory->CreateSwapChain(device.Get(), &swapChainDesc, swapChain.GetAddressOf()));

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