#include <pch.hpp>
#include <game/renderer.hpp>
#include <winUtils.hpp>
#include <engine/window.hpp>

#include <filesystem>

#define BUILD_DIR "./x64/Debug/"

Renderer::Renderer(Gfx& gfx) {
	fullscreenQuadPtVb = gfx.createVb(fullscreenQuadVerts, sizeof(fullscreenQuadVerts), sizeof(fullscreenQuadVerts[0]));
	fullscreenQuadIb = gfx.createIb(fullscreenQuadIndices, sizeof(fullscreenQuadIndices), sizeof(fullscreenQuadIndices[0]));

	ComPtr<ID3DBlob> vsBlob;
	CHECK_WIN_HRESULT(D3DReadFileToBlob(BUILD_DIR L"vsTransformQuadPt.cso", vsBlob.GetAddressOf()));
	CHECK_WIN_HRESULT(gfx.device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, transformQuadPtShader.GetAddressOf()));

	// Can use D3D10_APPEND_ALIGNED_ELEMENT instead of the offsetof
	const D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
		{
			.SemanticName = "Position",
			.SemanticIndex = 0,
			.Format = DXGI_FORMAT_R32G32_FLOAT,
			.InputSlot = 0,
			.AlignedByteOffset = offsetof(PtVert, pos),
			.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
			.InstanceDataStepRate = 0
		},
		{
			.SemanticName = "TexturePos",
			.SemanticIndex = 0,
			.Format = DXGI_FORMAT_R32G32_FLOAT,
			.InputSlot = 0,
			.AlignedByteOffset = offsetof(PtVert, texturePos),
			.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
			.InstanceDataStepRate = 0
		}
	};

	CHECK_WIN_HRESULT(gfx.device->CreateInputLayout(
		layoutDesc,
		static_cast<UINT>(std::size(layoutDesc)),
		vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(),
		ptLayout.GetAddressOf()
	));

	ComPtr<ID3DBlob> psBlob;
	CHECK_WIN_HRESULT(D3DReadFileToBlob(BUILD_DIR L"psCircle.cso", psBlob.GetAddressOf()));
	CHECK_WIN_HRESULT(gfx.device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, circleShader.GetAddressOf()));

	D3D11_BUFFER_DESC circleShaderConstatntBufferDesc{
		.ByteWidth = sizeof(CircleShaderConstantBuffer),
		.Usage = D3D11_USAGE_DYNAMIC,
		.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
		.MiscFlags = 0,
		.StructureByteStride = 0,
	};
	CHECK_WIN_HRESULT(gfx.device->CreateBuffer(&circleShaderConstatntBufferDesc, nullptr, circleShaderConstantBuffer.GetAddressOf()));

	const D3D11_RASTERIZER_DESC rasterizerDesc{
		.FillMode = D3D11_FILL_SOLID,
		.CullMode = D3D11_CULL_BACK,
		.FrontCounterClockwise = FALSE,
		.DepthBias = 0,
		.DepthBiasClamp = 0.0f,
		.SlopeScaledDepthBias = 0.0f,
		.DepthClipEnable = TRUE,
		.ScissorEnable = FALSE,
		.MultisampleEnable = TRUE,
		.AntialiasedLineEnable = TRUE
	};
	ComPtr<ID3D11RasterizerState> rasterizerState;
	CHECK_WIN_HRESULT(gfx.device->CreateRasterizerState(&rasterizerDesc, rasterizerState.GetAddressOf()));
	gfx.ctx->RSSetState(rasterizerState.Get());
	
	
	const D3D11_BLEND_DESC blendDesc{
		.AlphaToCoverageEnable = FALSE,
		.IndependentBlendEnable = FALSE,
		.RenderTarget {
			{
				.BlendEnable = TRUE,
				.SrcBlend = D3D11_BLEND_SRC_ALPHA,
				.DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
				.BlendOp = D3D11_BLEND_OP_ADD,
				.SrcBlendAlpha = D3D11_BLEND_ONE,
				.DestBlendAlpha = D3D11_BLEND_ZERO,
				.BlendOpAlpha = D3D11_BLEND_OP_ADD,
				.RenderTargetWriteMask = 0x0f,
			}
		}
	};
	ComPtr<ID3D11BlendState> blendState;
	CHECK_WIN_HRESULT(gfx.device->CreateBlendState(&blendDesc, blendState.GetAddressOf()));
	gfx.ctx->OMSetBlendState(blendState.Get(), nullptr, 0xffffffff);
}

auto Renderer::update(Gfx& gfx) -> void {
	gfx.ctx->OMSetRenderTargets(1, gfx.backBufferRenderTargetView.GetAddressOf(), nullptr);

	{
		float color[] = { 0.0f, 1.0f, 0.0f, 1.0f };
		gfx.ctx->ClearRenderTargetView(gfx.backBufferRenderTargetView.Get(), color);
	}

	gfx.ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	{
		const UINT stride{ sizeof(PtVert) }, offset{ 0 };
		gfx.ctx->IASetVertexBuffers(0, 1, fullscreenQuadPtVb.GetAddressOf(), &stride, &offset);
	}
	gfx.ctx->IASetIndexBuffer(fullscreenQuadIb.Get(), DXGI_FORMAT_R16_UINT, 0);

	if (Window::resized()) {
		const D3D11_VIEWPORT viewport{
			.TopLeftX = 0.0f,
			.TopLeftY = 0.0f,
			.Width = Window::size().x,
			.Height = Window::size().y,
			.MinDepth = 0.0f,
			.MaxDepth = 1.0f,
		};
		gfx.ctx->RSSetViewports(1, &viewport);
	}
	
	gfx.ctx->IASetInputLayout(ptLayout.Get());
	gfx.ctx->VSSetShader(transformQuadPtShader.Get(), nullptr, 0);
	gfx.ctx->PSSetShader(circleShader.Get(), nullptr, 0);

	D3D11_MAPPED_SUBRESOURCE resource{};
	CHECK_WIN_HRESULT(gfx.ctx->Map(circleShaderConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));

	// This may be bad if there is a lot of computation do here. I don't know the cost of Map.
	auto& buffer = *reinterpret_cast<CircleShaderConstantBuffer*>(resource.pData);
	memset(resource.pData, 0, sizeof(buffer));
	float sY = Window::size().x / Window::size().y;
	const auto s = Mat3x2::scale(Vec2{ 1.0, sY });
	buffer.instanceData[0] = { Mat3x2::scale(Vec2{ 0.4f }) * Mat3x2::rotate(0.3f) * s * Mat3x2::translate(Vec2{ 0.2f, 0.5f * sY }) };
	buffer.instanceData[1] = { Mat3x2::scale(Vec2{ 0.3f }) * s * Mat3x2::translate(Vec2{ -0.3f, -0.3f * sY }) };

	//memcpy(resource.pData, &buffer, sizeof(buffer));
	gfx.ctx->Unmap(circleShaderConstantBuffer.Get(), 0);

	gfx.ctx->VSSetConstantBuffers(0, 1, circleShaderConstantBuffer.GetAddressOf());
	gfx.ctx->DrawIndexedInstanced(static_cast<UINT>(std::size(fullscreenQuadIndices)), 2, 0, 0, 0);
}
