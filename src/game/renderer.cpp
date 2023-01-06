#include <pch.hpp>
#include <game/renderer.hpp>
#include <game/debug.hpp>
#include <math/utils.hpp>
#include <winUtils.hpp>
#include <engine/window.hpp>
#include <stdio.h>
#include <cmath>

#define BUILD_DIR "./x64/Debug/"

#include <imgui/imgui.h>

Renderer::Renderer(Gfx& gfx) {
	vsCircle = gfx.vsFromFile(BUILD_DIR L"vsCircle.cso");
	psCircleCollider = gfx.psFromFile(BUILD_DIR L"psCircleCollider.cso");
	psCircle = gfx.psFromFile(BUILD_DIR L"psCircle.cso");
	psHollowCircle = gfx.psFromFile(BUILD_DIR L"psHollowCircle.cso");
	circleShaderConstantBufferResource = gfx.createConstantBuffer(sizeof(circleShaderConstantBuffer));

	vsLine = gfx.vsFromFile(BUILD_DIR L"vsLine.cso");
	psLine = gfx.psFromFile(BUILD_DIR L"psLine.cso");
	lineShaderConstantBufferResource = gfx.createConstantBuffer(sizeof(lineShaderConstantBuffer));

	vsParabola = gfx.vsFromFile(BUILD_DIR L"vsParabola.cso");
	psParabola = gfx.psFromFile(BUILD_DIR L"psParabola.cso");
	parabolaShaderConstantBufferResource = gfx.createConstantBuffer(sizeof(parabolaShaderConstantBuffer));

	vsTexturedQuad = gfx.vsFromFile(BUILD_DIR L"vsTexturedQuad.cso");
	psTexturedQuad = gfx.psFromFile(BUILD_DIR L"psTexturedQuad.cso");
	texturedQuadConstantBufferResource = gfx.createConstantBuffer(sizeof(texturedQuadConstantBuffer));

	debugShapesFragmentShaderConstantBufferResource = gfx.createConstantBuffer(sizeof(debugShapesFragmentShaderConstantBuffer));
	sizeof(parabolaShaderConstantBuffer);
	{
		fullscreenQuadPtVb = gfx.createVb(fullscreenQuadVerts, sizeof(fullscreenQuadVerts), sizeof(fullscreenQuadVerts[0]));
		fullscreenQuadIb = gfx.createIb(fullscreenQuadIndices, sizeof(fullscreenQuadIndices), sizeof(fullscreenQuadIndices[0]));
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
			vsCircle.blob->GetBufferPointer(),
			vsCircle.blob->GetBufferSize(),
			ptLayout.GetAddressOf()
		));
	}

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
				.SrcBlendAlpha = D3D11_BLEND_ZERO,
				.DestBlendAlpha = D3D11_BLEND_ONE,
				.BlendOpAlpha = D3D11_BLEND_OP_ADD,
				.RenderTargetWriteMask = 0x0f,
			}
		}
	};
	ComPtr<ID3D11BlendState> blendState;
	CHECK_WIN_HRESULT(gfx.device->CreateBlendState(&blendDesc, blendState.GetAddressOf()));
	gfx.ctx->OMSetBlendState(blendState.Get(), nullptr, 0xffffffff);

	{
		const D3D11_TEXTURE2D_DESC textureDesc{
			.Width = static_cast<UINT>(textureSize.x),
			.Height = static_cast<UINT>(textureSize.y),
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = DXGI_FORMAT_B8G8R8A8_UNORM,
			.SampleDesc = {
				.Count = 1,
				.Quality = 0,
			},
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};
		CHECK_WIN_HRESULT(gfx.device->CreateTexture2D(&textureDesc, nullptr, windowTexture.GetAddressOf()));

		const D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc{
			.Format = textureDesc.Format,
			.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D,
			.Texture2D = {
				.MipSlice = 0
			}
		};
		CHECK_WIN_HRESULT(gfx.device->CreateRenderTargetView(windowTexture.Get(), &renderTargetViewDesc, windowTextureRenderTargetView.GetAddressOf()));

		const D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{
			.Format = textureDesc.Format,
			.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
			.Texture2D = {
				.MostDetailedMip = 0,
				.MipLevels = 1,
			}
		};
		CHECK_WIN_HRESULT(gfx.device->CreateShaderResourceView(windowTexture.Get(), &shaderResourceViewDesc, windowTextureShaderResourceView.GetAddressOf()));
	}

	{
		const D3D11_SAMPLER_DESC samplerDesc{
			.Filter = D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT,
			.AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
			.ComparisonFunc = D3D11_COMPARISON_NEVER,
			.MinLOD = 0,
			.MaxLOD = D3D11_FLOAT32_MAX,
		};
		CHECK_WIN_HRESULT(gfx.device->CreateSamplerState(&samplerDesc, pixelTextureSamplerState.GetAddressOf()));
	}
}

#include <utils/io.hpp>

auto Renderer::update(Gfx& gfx, const Camera& camera, Vec2 windowSize, bool renderToTexture) -> void {

	const D3D11_VIEWPORT viewport{
		.TopLeftX = 0.0f,
		.TopLeftY = 0.0f,
		.Width = windowSize.x,
		.Height = windowSize.y,
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f,
	};
	gfx.ctx->RSSetViewports(1, &viewport);

	if (renderToTexture) {
		float color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		gfx.ctx->ClearRenderTargetView(windowTextureRenderTargetView.Get(), color);
		gfx.ctx->OMSetRenderTargets(1, windowTextureRenderTargetView.GetAddressOf(), nullptr);
	} else {
		gfx.ctx->OMSetRenderTargets(1, gfx.backBufferRenderTargetView.GetAddressOf(), nullptr);
	}

	{
		float color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		gfx.ctx->ClearRenderTargetView(gfx.backBufferRenderTargetView.Get(), color);
	}

	gfx.ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	{
		const UINT stride{ sizeof(PtVert) }, offset{ 0 };
		gfx.ctx->IASetVertexBuffers(0, 1, fullscreenQuadPtVb.GetAddressOf(), &stride, &offset);
	}

	gfx.ctx->IASetIndexBuffer(fullscreenQuadIb.Get(), DXGI_FORMAT_R16_UINT, 0);
	gfx.ctx->IASetInputLayout(ptLayout.Get());

	const auto aspectRatio{ windowSize.x / windowSize.y };
	const auto screenScale{ Mat3x2::scale(Vec2{ 1.0f, aspectRatio }) };

	const auto cameraTransform = camera.cameraTransform();

	auto makeTransform = [&screenScale, aspectRatio, &cameraTransform](Vec2 translation, float orientation, float scale) -> Mat3x2 {
		return Mat3x2::rotate(orientation) * screenScale * Mat3x2::scale(Vec2{ scale }) * Mat3x2::translate(Vec2{ translation.x, translation.y * aspectRatio }) * cameraTransform;
	};

	{
		gfx.updateConstantBuffer(texturedQuadConstantBufferResource, &texturedQuadConstantBuffer, sizeof(texturedQuadConstantBuffer));
		gfx.ctx->VSSetShader(vsTexturedQuad.shader.Get(), nullptr, 0);

		gfx.ctx->PSSetShader(psTexturedQuad.Get(), nullptr, 0);
		gfx.ctx->PSSetSamplers(0, 1, pixelTextureSamplerState.GetAddressOf());

		for (auto& [texture, pos, size] : dynamicTexturesToDraw) {
			gfx.ctx->PSSetShaderResources(0, 1, texture->resourceView.GetAddressOf());
			texturedQuadConstantBuffer.transform = makeTransform(pos, 0.0f, size);
			gfx.ctx->VSSetConstantBuffers(0, 1, texturedQuadConstantBufferResource.GetAddressOf());

			D3D11_MAPPED_SUBRESOURCE resource{ 0 };
			CHECK_WIN_HRESULT(gfx.ctx->Map(texture->texture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));

			for (usize y = 0; y < texture->size().y; y++) {
				auto rowDst = reinterpret_cast<u8*>(resource.pData) + y * resource.RowPitch;
				const auto rowByteWidth = texture->size().x * sizeof(u32);
				auto rowSrc = reinterpret_cast<u8*>(texture->data()) + rowByteWidth * y;
				memcpy(rowDst, rowSrc, rowByteWidth);
			}

			gfx.ctx->Unmap(texture->texture.Get(), 0);

			// Assumes the fullscreen quad index buffer is set.
			gfx.ctx->DrawIndexed(static_cast<UINT>(std::size(fullscreenQuadIndices)), 0, 0);
		}
		dynamicTexturesToDraw.clear();

	}

	debugShapesFragmentShaderConstantBuffer.lineWidth = 0.003f * 1920.0f / windowSize.x;
	debugShapesFragmentShaderConstantBuffer.smoothingWidth = debugShapesFragmentShaderConstantBuffer.lineWidth * (2.0f / 3.0f);
	gfx.updateConstantBuffer(debugShapesFragmentShaderConstantBufferResource, &debugShapesFragmentShaderConstantBuffer, sizeof(debugShapesFragmentShaderConstantBuffer));

	// checkDraw can be called last in loops because zero sized arrays are not allowed.
	// remember to put a draw or checkDraw after a loop.
		// Circle
	// !!!!!!!!!!!!! TODO: Make a templated class or a macro for dealing with drawing debug shapes. Later specifying the width might be needed.
	{
		gfx.ctx->VSSetShader(vsCircle.shader.Get(), nullptr, 0);

		gfx.ctx->PSSetShader(psCircleCollider.Get(), nullptr, 0);
		UINT toDraw = 0;
		auto draw = [&] {
			if (toDraw == 0)
				return;
			gfx.updateConstantBuffer(circleShaderConstantBufferResource, &circleShaderConstantBuffer, sizeof(circleShaderConstantBuffer));
			gfx.ctx->DrawIndexedInstanced(static_cast<UINT>(std::size(fullscreenQuadIndices)), toDraw, 0, 0, 0);
			toDraw = 0;
		};
		auto checkDraw = [&] {
			if (toDraw >= std::size(lineShaderConstantBuffer.instanceData)) {
				draw();
			}
		};

		gfx.ctx->VSSetConstantBuffers(0, 1, circleShaderConstantBufferResource.GetAddressOf());
		for (const auto& [circle, orientation] : Debug::circleColliders) {
			circleShaderConstantBuffer.instanceData[toDraw] = {
				.invRadius = 1.0f / circle.radius / camera.zoom,
				.transform = makeTransform(circle.pos, orientation, circle.radius),
				.color = circle.color
			};
			toDraw++;
			checkDraw();
		}
		draw();

		gfx.ctx->PSSetShader(psCircle.Get(), nullptr, 0);
		for (const auto& circle : Debug::circles) {
			circleShaderConstantBuffer.instanceData[toDraw] = {
				.invRadius = 1.0f / circle.radius / camera.zoom,
				.transform = makeTransform(circle.pos, 0.0f, circle.radius),
				.color = circle.color
			};
			toDraw++;
			checkDraw();
		}
		draw();

		gfx.ctx->PSSetShader(psCircle.Get(), nullptr, 0);
		for (const auto& point : Debug::points) {
			circleShaderConstantBuffer.instanceData[toDraw] = {
				.invRadius = 1.0f / point.radius,
				.transform = makeTransform(point.pos, 0.0f, point.radius / camera.zoom),
				.color = point.color
			};
			toDraw++;
			checkDraw();
		}
		draw();

		gfx.ctx->PSSetShader(psHollowCircle.Get(), nullptr, 0);
		for (const auto& hollowCircle : Debug::hollowCircles) {
			circleShaderConstantBuffer.instanceData[toDraw] = {
				.invRadius = 1.0f / hollowCircle.radius / camera.zoom,
				.transform = makeTransform(hollowCircle.pos, 0.0f, hollowCircle.radius),
				.color = hollowCircle.color
			};
			toDraw++;
			checkDraw();
		}
		draw();
	}

	// Line
	{
		gfx.ctx->VSSetShader(vsLine.shader.Get(), nullptr, 0);
		gfx.ctx->PSSetShader(psLine.Get(), nullptr, 0);
		gfx.ctx->PSSetConstantBuffers(0, 1, debugShapesFragmentShaderConstantBufferResource.GetAddressOf());
		UINT toDraw = 0;
		auto draw = [&] {
			if (toDraw == 0)
				return;
			gfx.updateConstantBuffer(lineShaderConstantBufferResource, &lineShaderConstantBuffer, sizeof(lineShaderConstantBuffer));
			gfx.ctx->DrawIndexedInstanced(static_cast<UINT>(std::size(fullscreenQuadIndices)), toDraw, 0, 0, 0);
			toDraw = 0;
		};
		auto checkDraw = [&] {
			if (toDraw >= std::size(lineShaderConstantBuffer.instanceData)) {
				draw();
			}
		};
		const auto extraLength = debugShapesFragmentShaderConstantBuffer.lineWidth / 2.0f / camera.zoom; // Make the line longer by the width of the line so both ends are rounded. Don't think I can do it inside the vertex shader because of the order in which the transforms have to be applied.

		gfx.ctx->VSSetConstantBuffers(0, 1, lineShaderConstantBufferResource.GetAddressOf());

		auto lineInstanceFromStartAndEnd = [&](Vec2 start, Vec2 end, const Vec3& color) -> LineInstance {
			const auto lineVector = end - start;
			const auto length = lineVector.length();
			const auto orientation = atan2(lineVector.y, lineVector.x);
			return {
				.invScale = 1.0f / length / camera.zoom,
				.transform = makeTransform(start, orientation, length + extraLength),
				.color = color,
			};
		};

		for (const auto& line : Debug::lines) {
			const auto lineVector = line.end - line.start;
			const auto length = lineVector.length();
			const auto orientation = atan2(lineVector.y, lineVector.x);
			lineShaderConstantBuffer.instanceData[toDraw] = lineInstanceFromStartAndEnd(line.start, line.end, line.color);
			toDraw++;
			checkDraw();
		}
		checkDraw();

		draw();
	}

	// Parabola
	{
		gfx.ctx->VSSetShader(vsParabola.shader.Get(), nullptr, 0);
		gfx.ctx->PSSetShader(psParabola.Get(), nullptr, 0);
		UINT toDraw = 0;
		auto draw = [&] {
			if (toDraw == 0)
				return;
			gfx.updateConstantBuffer(parabolaShaderConstantBufferResource, &parabolaShaderConstantBuffer, sizeof(parabolaShaderConstantBuffer));
			gfx.ctx->DrawIndexedInstanced(static_cast<UINT>(std::size(fullscreenQuadIndices)), toDraw, 0, 0, 0);
			toDraw = 0;
		};
		auto checkDraw = [&] {
			if (toDraw >= std::size(parabolaShaderConstantBuffer.instanceData)) {
				draw();
			}
		};

		gfx.ctx->VSSetConstantBuffers(0, 1, parabolaShaderConstantBufferResource.GetAddressOf());

		for (const auto& parabola : Debug::parabolas) {
			parabolaShaderConstantBuffer.instanceData[toDraw] = {
				.invScale = 1.0f / camera.zoom,
				.transform = Mat3x2::scale(Vec2{ 1.0f / camera.zoom, -1.0f / camera.aspectRatio / camera.zoom })
					* Mat3x2::translate(camera.pos * Vec2{ 1.0 / 2.0f, 1.0f / 2.0f }),
				.color = Vec3(1.0),
				.cofefficients = Vec3{ 2.0f * parabola.a, parabola.pos.x / 2.0f , parabola.pos.y / 2.0f }
			};
			toDraw++;
			checkDraw();
		}
		checkDraw();

		draw();
	}

	if (renderToTexture) {
		gfx.ctx->OMSetRenderTargets(1, gfx.backBufferRenderTargetView.GetAddressOf(), nullptr);
	}
}

auto Renderer::drawDynamicTexture(Vec2 pos, float size, DynamicTexture& dynamicTexture) -> void {
	dynamicTexturesToDraw.push_back(DynamicTextureToDraw{
		.texture = &dynamicTexture,
		.pos = pos,
		.size = size,
	});
}

DynamicTexture::DynamicTexture(Gfx& gfx, Vec2T<i32> size)
	: size_{ size }
	, data_{ new u32[static_cast<usize>(size.x) * static_cast<usize>(size.y)] } {

	const D3D11_TEXTURE2D_DESC textureDesc{
		.Width = static_cast<UINT>(size_.x),
		.Height = static_cast<UINT>(size_.y),
		.MipLevels = 1,
		.ArraySize = 1,
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.SampleDesc = {
			.Count = 1,
			.Quality = 0,
		},
		.Usage = D3D11_USAGE_DYNAMIC,
		.BindFlags = D3D11_BIND_SHADER_RESOURCE,
		.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
		.MiscFlags = 0,
	};
	CHECK_WIN_HRESULT(gfx.device->CreateTexture2D(&textureDesc, nullptr, texture.GetAddressOf()));

	const D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{
		.Format = textureDesc.Format,
		.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
		.Texture2D = {
			.MostDetailedMip = 0,
			.MipLevels = 1,
		}
	};
	CHECK_WIN_HRESULT(gfx.device->CreateShaderResourceView(texture.Get(), &shaderResourceViewDesc, resourceView.GetAddressOf()));

	for (usize i = 0; i < static_cast<usize>(size.x) * static_cast<usize>(size.y); i++) {
		data()[i] = 0xFFFFFFFF;
	}
}

DynamicTexture::~DynamicTexture() {
	delete[] data_;
}

auto DynamicTexture::set(Vec2T<i32> pos, const Vec3T<u8>& color) -> void {
	data_[pos.y * size_.x + pos.x] = color.x | (color.y << 8) | (color.z << 16);
}

auto DynamicTexture::set(Vec2T<i32> pos, const Vec3& color) -> void {
	const Vec3T<u8> colU8{
		static_cast<u8>(std::clamp(color.x * 255.0f, 0.0f, 255.0f)),
		static_cast<u8>(std::clamp(color.y * 255.0f, 0.0f, 255.0f)),
		static_cast<u8>(std::clamp(color.z * 255.0f, 0.0f, 255.0f))
	};
	set(pos, colU8);
}

auto DynamicTexture::get(Vec2T<i32> pos) const -> Vec3T<u8> {
	const auto value = data_[pos.y * size_.x + pos.x];
	return { static_cast<u8>(value), static_cast<u8>(value >> 8), static_cast<u8>(value >> 16) };
}

auto DynamicTexture::size() const -> const Vec2T<i32>& {
	return size_;
}

auto DynamicTexture::data() -> u32* {
	return data_;
}
