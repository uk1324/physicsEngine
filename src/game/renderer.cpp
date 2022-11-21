#include <pch.hpp>
#include <game/renderer.hpp>
#include <game/debug.hpp>
#include <math/utils.hpp>
#include <winUtils.hpp>
#include <engine/window.hpp>
#include <stdio.h>
#include <cmath>

#define BUILD_DIR "./x64/Debug/"

Renderer::Renderer(Gfx& gfx) {
	vsCircle = gfx.vsFromFile(BUILD_DIR L"vsCircle.cso");
	psCircleCollider = gfx.psFromFile(BUILD_DIR L"psCircleCollider.cso");
	psCircle = gfx.psFromFile(BUILD_DIR L"psCircle.cso");
	circleShaderConstantBufferResource = gfx.createConstantBuffer(sizeof(circleShaderConstantBuffer));

	vsLine = gfx.vsFromFile(BUILD_DIR L"vsLine.cso");
	psLine = gfx.psFromFile(BUILD_DIR L"psLine.cso");
	lineShaderConstantBufferResource = gfx.createConstantBuffer(sizeof(lineShaderConstantBuffer));

	vsParabola = gfx.vsFromFile(BUILD_DIR L"vsParabola.cso");
	psParabola = gfx.psFromFile(BUILD_DIR L"psParabola.cso");
	parabolaShaderConstantBufferResource = gfx.createConstantBuffer(sizeof(parabolaShaderConstantBuffer));

	debugShapesFragmentShaderConstantBufferResource = gfx.createConstantBuffer(sizeof(debugShapesFragmentShaderConstantBuffer));

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

#include <utils/io.hpp>

auto Renderer::update(Gfx& gfx, const Camera& camera) -> void {
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

	gfx.ctx->OMSetRenderTargets(1, gfx.backBufferRenderTargetView.GetAddressOf(), nullptr);

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

	const auto aspectRatio{ Window::size().x / Window::size().y };
	const auto screenScale{ Mat3x2::scale(Vec2{ 1.0f, aspectRatio }) };

	const auto cameraTransform = camera.cameraTransform();

	auto makeTransform = [&screenScale, aspectRatio, &cameraTransform](Vec2 translation, float orientation, float scale) -> Mat3x2 {
		return Mat3x2::rotate(orientation) * screenScale * Mat3x2::scale(Vec2{ scale }) * Mat3x2::translate(Vec2{ translation.x, translation.y * aspectRatio }) * cameraTransform;
	};

	debugShapesFragmentShaderConstantBuffer.lineWidth = 0.003f * 1920.0f / Window::size().x;
	debugShapesFragmentShaderConstantBuffer.smoothingWidth = debugShapesFragmentShaderConstantBuffer.lineWidth * (2.0f / 3.0f);
	gfx.updateConstantBuffer(debugShapesFragmentShaderConstantBufferResource, &debugShapesFragmentShaderConstantBuffer, sizeof(debugShapesFragmentShaderConstantBuffer));

	// checkDraw can be called last in loops because zero sized arrays are not allowed.
	// remember to put a draw or checkDraw after a loop.

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

	// Circle
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
		for (const auto& [circle, orientation]: Debug::emptyCircles) {
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
}

Camera::Camera(Vec2 pos, float zoom) 
	: pos{ pos }
	, zoom{ zoom } {}

auto Camera::interpolateTo(Vec2 desiredPos, float speed) -> void {
	pos = lerp(pos, desiredPos, speed);
}

auto Camera::cameraTransform() const -> Mat3x2 {
	return Mat3x2::translate(-pos * Vec2{ 1.0f, aspectRatio }) * Mat3x2::scale(Vec2{ zoom });
}

auto Camera::screenSpaceToCameraSpace(Vec2 screenSpacePos) -> Vec2 {
	return (screenSpacePos * Vec2{ 1.0f, 1.0f / aspectRatio } / zoom) + pos;
}
