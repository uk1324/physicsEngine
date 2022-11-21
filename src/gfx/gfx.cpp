#include <pch.hpp>
#include <gfx/gfx.hpp>
#include <engine/window.hpp>
#include <winUtils.hpp>
#include <utils/io.hpp>

#include <inttypes.h>

#pragma comment(lib, "dxgi.lib")

Gfx::Gfx(HWND hWnd_) {
	CHECK_WIN_HRESULT(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION, device.GetAddressOf(), nullptr, ctx.GetAddressOf()));

	DXGI_SAMPLE_DESC sample{ .Count = 1, .Quality = 0 };
	for (UINT i = sample.Count; i <= 8; i++) {
		UINT quality;
		CHECK_WIN_HRESULT(device->CheckMultisampleQualityLevels(SWAP_CHAIN_FORMAT, i, &quality));
		if (quality > 0) {
			sample.Count = i;
			sample.Quality = quality - 1;
		}
	}
	LOG_INFO("MSAA using count: %" PRIu32 " quality: % " PRIu32, sample.Count, sample.Quality);

	DXGI_SWAP_CHAIN_DESC swapChainDesc{
		.BufferDesc = {
			.Width = 0,
			.Height = 0,
			.RefreshRate {
				.Numerator = 0,
				.Denominator = 0,
			},
			.Format = SWAP_CHAIN_FORMAT,
			.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
			.Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
		},
		.SampleDesc = sample,
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = SWAP_CHAIN_BUFFER_COUNT,
		.OutputWindow = hWnd_,
		.Windowed = TRUE,
		.SwapEffect = DXGI_SWAP_EFFECT_DISCARD,
		.Flags = SWAP_CHAIN_FLAGS,
	};
	ComPtr<IDXGIFactory> factory;
	CHECK_WIN_HRESULT(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(factory.GetAddressOf())));
	// TODO: Using CreateSwapChain is not recommended. Look at documentation. Maybe change later.
	CHECK_WIN_HRESULT(factory->CreateSwapChain(device.Get(), &swapChainDesc, swapChain.GetAddressOf()));

	//D3D11_FEATURE_DATA_DOUBLES doublesSupport;
	//doublesSupport.DoublePrecisionFloatShaderOps = TRUE;
	//// TODO: Do error handling if not supported.
	//CHECK_WIN_HRESULT(device->CheckFeatureSupport(D3D11_FEATURE_DOUBLES, &doublesSupport, sizeof(D3D11_FEATURE_DATA_DOUBLES)));
	//if (doublesSupport.DoublePrecisionFloatShaderOps == TRUE) {
	//	LOG_INFO("double precision supported");
	//}

	setBackBufferRenderTargetView();
}

auto Gfx::update() -> void {
	if (Window::resized()) {
		backBufferRenderTargetView->Release();
		CHECK_WIN_HRESULT(swapChain->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT, static_cast<UINT>(Window::size().x), static_cast<UINT>(Window::size().y), SWAP_CHAIN_FORMAT, SWAP_CHAIN_FLAGS));

		setBackBufferRenderTargetView();
	}
}

auto Gfx::present() -> void {
	if (const auto presentResult{ swapChain->Present(1, 0) }; presentResult == ERROR_DEVICE_REMOVED)
		CHECK_WIN_HRESULT(device->GetDeviceRemovedReason());
}

auto Gfx::createConstantBuffer(UINT sizeBytes) -> ComPtr<ID3D11Buffer> {
	D3D11_BUFFER_DESC bufferDesc{
		.ByteWidth = sizeBytes,
		.Usage = D3D11_USAGE_DYNAMIC,
		.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
		.MiscFlags = 0,
		.StructureByteStride = 0,
	};
	ComPtr<ID3D11Buffer> buffer;
	CHECK_WIN_HRESULT(device->CreateBuffer(&bufferDesc, nullptr, buffer.GetAddressOf()));
	return buffer;
}

auto Gfx::updateConstantBuffer(ComPtr<ID3D11Buffer>& buffer, void* data, usize dataSize) -> void {
	D3D11_MAPPED_SUBRESOURCE resource{ 0 };
	CHECK_WIN_HRESULT(ctx->Map(buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));
	// Copying here instead of getting the pointer and writing to it because while a resource is Mapped it cannot be accessed by the GPU.
	memcpy(resource.pData, data, dataSize);
	ctx->Unmap(buffer.Get(), 0);
}

auto Gfx::vsFromFile(LPCWSTR filename) -> VertexShader {
	ComPtr<ID3D11VertexShader> vs;
	ComPtr<ID3DBlob> vsBlob;
	CHECK_WIN_HRESULT(D3DReadFileToBlob(filename, &vsBlob));
	CHECK_WIN_HRESULT(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vs));
	return VertexShader{ std::move(vs), std::move(vsBlob) };
}

auto Gfx::psFromFile(LPCWSTR filename) -> ComPtr<ID3D11PixelShader> {
	ComPtr<ID3D11PixelShader> ps;
	ComPtr<ID3DBlob> psBlob;
	CHECK_WIN_HRESULT(D3DReadFileToBlob(filename, psBlob.GetAddressOf()));
	CHECK_WIN_HRESULT(device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &ps));
	return ps;
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

auto Gfx::setBackBufferRenderTargetView() -> void {
	ComPtr<ID3D11Resource> backBuffer;
	CHECK_WIN_HRESULT(swapChain->GetBuffer(
		0 /* 0 is the back buffer */,
		__uuidof(ID3D11Resource),
		reinterpret_cast<void**>(backBuffer.GetAddressOf())
	));
	CHECK_WIN_HRESULT(device->CreateRenderTargetView(backBuffer.Get(), nullptr, backBufferRenderTargetView.GetAddressOf()));
}
