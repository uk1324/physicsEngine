#include <pch.hpp>
#include <engine/dx11Renderer.hpp>
#include <engine/window.hpp>
#include <engine/debug.hpp>
#include <imgui/imgui.h>
#include <winUtils.hpp>

#ifdef _DEBUG
#define BUILD_DIR "./x64/Debug/"
#else
#define BUILD_DIR "./x64/Release/"
#endif


Dx11Renderer::Dx11Renderer(Gfx& gfx)
	: gfx{ gfx } {
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

	vsFullscreenQuad = gfx.vsFromFile(BUILD_DIR L"vsFullscreenQuad.cso");
	psGrid = gfx.psFromFile(BUILD_DIR L"psGrid.cso");
	fullscreenQuadConstantBufferResource = gfx.createConstantBuffer(sizeof(fullscreenQuadConstantBuffer));
	gridConstantBufferResource = gfx.createConstantBuffer(sizeof(gridConstantBuffer));

	debugShapesFragmentShaderConstantBufferResource = gfx.createConstantBuffer(sizeof(debugShapesFragmentShaderConstantBuffer));
	{
		const D3D11_BUFFER_DESC desc{
			.ByteWidth = static_cast<UINT>(sizeof(PcVert) * DYNAMIC_TRIANGLES_SIZE),
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = static_cast<UINT>(D3D11_BIND_VERTEX_BUFFER),
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
			.MiscFlags = 0,
			.StructureByteStride = sizeof(PcVert),
		};
		CHECK_WIN_HRESULT(gfx.device->CreateBuffer(&desc, nullptr, dynamicTriangles.GetAddressOf()));
		const D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
			{
				.SemanticName = "Position",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R32G32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = offsetof(PcVert, pos),
				.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},
			{
				.SemanticName = "Color",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = offsetof(PcVert, color),
				.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			}
		};

		vsColoredTriangle = gfx.vsFromFile(BUILD_DIR L"vsColoredTriangle.cso");
		psColoredTriangle = gfx.psFromFile(BUILD_DIR L"psColoredTriangle.cso");

		CHECK_WIN_HRESULT(gfx.device->CreateInputLayout(
			layoutDesc,
			static_cast<UINT>(std::size(layoutDesc)),
			vsColoredTriangle.blob->GetBufferPointer(),
			vsColoredTriangle.blob->GetBufferSize(),
			pcLayout.GetAddressOf()
		));
	}

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
		D3D11_SAMPLER_DESC samplerDesc{
			.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
			.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
			.ComparisonFunc = D3D11_COMPARISON_NEVER,
			.MinLOD = 0,
			.MaxLOD = D3D11_FLOAT32_MAX,
		};
		CHECK_WIN_HRESULT(gfx.device->CreateSamplerState(&samplerDesc, nearestNeighbourTextureSamplerState.GetAddressOf()));

		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		CHECK_WIN_HRESULT(gfx.device->CreateSamplerState(&samplerDesc, linearTextureSamplerState.GetAddressOf()));
	}
}

#include <utils/io.hpp>

auto Dx11Renderer::update(Camera& camera, std::optional<float> gridSmallCellSize, std::optional<Vec2> windowSizeIfRenderingToTexture) -> void {
	const auto windowSize = windowSizeIfRenderingToTexture.has_value() ? *windowSizeIfRenderingToTexture : Window::size();
	camera.aspectRatio = windowSize.xOverY();

	const D3D11_VIEWPORT viewport{
		.TopLeftX = 0.0f,
		.TopLeftY = 0.0f,
		.Width = windowSize.x,
		.Height = windowSize.y,
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f,
	};
	gfx.ctx->RSSetViewports(1, &viewport);

	const auto renderToTexture = windowSizeIfRenderingToTexture.has_value();
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

	const auto aspectRatio{ windowSize.x / windowSize.y };
	const auto screenScale{ Mat3x2::scale(Vec2{ 1.0f, aspectRatio }) };

	const auto cameraTransform = camera.cameraTransform();

	auto makeTransform = [&screenScale, aspectRatio, &cameraTransform](Vec2 translation, float orientation, Vec2 scale) -> Mat3x2 {
		return Mat3x2::rotate(orientation) * screenScale * Mat3x2::scale(scale) * Mat3x2::translate(Vec2{ translation.x, translation.y * aspectRatio }) * cameraTransform;
	};
	const auto transformForPoints = screenScale * cameraTransform;

	auto x = transformForPoints * transformForPoints.inversed();

	auto cameraSpaceToWindowSpace = [&windowSize, &camera](Vec2 p) {
		p = camera.cameraSpaceToScreenSpace(p);
		p += Vec2{ 1.0f, -1.0f };
		p /= 2.0f;
		p = p.flippedY();
		p *= windowSize;
		return p;
	};

	if (gridSmallCellSize.has_value()) {
		fullscreenQuadConstantBuffer.transform = (screenScale * cameraTransform).inversed();
		gfx.updateConstantBuffer(fullscreenQuadConstantBufferResource, &fullscreenQuadConstantBuffer, sizeof(fullscreenQuadConstantBuffer));
		gfx.ctx->VSSetConstantBuffers(0, 1, fullscreenQuadConstantBufferResource.GetAddressOf());

		gridConstantBuffer.cameraZoom = camera.zoom;
		gridConstantBuffer.smallCellSize = *gridSmallCellSize;
		gfx.updateConstantBuffer(gridConstantBufferResource, &gridConstantBuffer, sizeof(gridConstantBuffer));
		gfx.ctx->PSSetConstantBuffers(0, 1, gridConstantBufferResource.GetAddressOf());

		gfx.ctx->VSSetShader(vsFullscreenQuad.shader.Get(), nullptr, 0);
		gfx.ctx->PSSetShader(psGrid.Get(), nullptr, 0);

		// Assumes the fullscreen quad index buffer is set.
		gfx.ctx->DrawIndexed(static_cast<UINT>(std::size(fullscreenQuadIndices)), 0, 0);
	}

	// Text
	{
		auto drawList = ImGui::GetBackgroundDrawList();
		for (const auto& [pos, text, color, height] : Debug::text) {
			// Not sure why does it need to be multiplied by 2. It doesn't need to divided by 2 because it is a size and not a position.
			const auto fontSizeWindowSpace = height / aspectRatio * camera.zoom * windowSize.y * 2.0f;

			// CalcTextSize uses GetFontSize() as the font size so it has to be converted.
			const auto centerOffset = Vec2{ ImGui::CalcTextSize(text) } / 2.0f * (fontSizeWindowSpace / ImGui::GetFontSize());
			const auto screenSpacePos = cameraSpaceToWindowSpace(pos) - centerOffset;
			const auto col = IM_COL32(color.x * 255.0f, color.y * 255.0f, color.z * 255.0f, 255);
			drawList->AddText(ImGui::GetFont(), fontSizeWindowSpace, screenSpacePos, col, text);
		}
	}

	// Dynamic textures
	{
		gfx.updateConstantBuffer(texturedQuadConstantBufferResource, &texturedQuadConstantBuffer, sizeof(texturedQuadConstantBuffer));
		gfx.ctx->VSSetShader(vsTexturedQuad.shader.Get(), nullptr, 0);

		gfx.ctx->PSSetShader(psTexturedQuad.Get(), nullptr, 0);
		for (auto& [texture, pos, size, interpolate] : dynamicTexturesToDraw) {
			if (interpolate) {
				gfx.ctx->PSSetSamplers(0, 1, linearTextureSamplerState.GetAddressOf());
			} else {
				gfx.ctx->PSSetSamplers(0, 1, nearestNeighbourTextureSamplerState.GetAddressOf());
			}

			auto& textureData = handleToDynamicTextureData[texture->textureHandle];
			gfx.ctx->PSSetShaderResources(0, 1, textureData.resourceView.GetAddressOf());
			const auto scale = Vec2{ (static_cast<float>(texture->size().x) / static_cast<float>(texture->size().y)) * (size / 2.0f), size / 2.0f };
			texturedQuadConstantBuffer.transform = makeTransform(pos, 0.0f, scale);
			gfx.ctx->VSSetConstantBuffers(0, 1, texturedQuadConstantBufferResource.GetAddressOf());

			D3D11_MAPPED_SUBRESOURCE resource{ 0 };
			CHECK_WIN_HRESULT(gfx.ctx->Map(textureData.texture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource));

			for (i64 y = 0; y < texture->size().y; y++) {
				auto rowDst = reinterpret_cast<u8*>(resource.pData) + y * resource.RowPitch;
				const auto rowByteWidth = texture->size().x * sizeof(u32);
				auto rowSrc = reinterpret_cast<u8*>(texture->data()) + rowByteWidth * y;
				memcpy(rowDst, rowSrc, rowByteWidth);
			}

			gfx.ctx->Unmap(textureData.texture.Get(), 0);

			// Assumes the fullscreen quad index buffer is set.
			gfx.ctx->DrawIndexed(static_cast<UINT>(std::size(fullscreenQuadIndices)), 0, 0);
		}
		dynamicTexturesToDraw.clear();

	}

	// Dynamic triangles.
	{
		gfx.ctx->VSSetShader(vsColoredTriangle.shader.Get(), nullptr, 0);
		gfx.ctx->PSSetShader(psColoredTriangle.Get(), nullptr, 0);

		gfx.ctx->IASetInputLayout(pcLayout.Get());

		const UINT stride{ sizeof(PcVert) }, offset{ 0 };
		gfx.ctx->IASetVertexBuffers(0, 1, dynamicTriangles.GetAddressOf(), &stride, &offset);

		UINT toDraw = 0;
		D3D11_MAPPED_SUBRESOURCE resource{ 0 };
		gfx.ctx->Map(dynamicTriangles.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
		auto data = reinterpret_cast<PcVert*>(resource.pData);

		for (const auto& [triangle, color] : Debug::triangles) {
			// Clockwise ordered triangles.
			data[toDraw] = PcVert{ triangle.v[0] * transformForPoints, color };
			toDraw++;
			data[toDraw] = PcVert{ triangle.v[1] * transformForPoints, color };
			toDraw++;
			data[toDraw] = PcVert{ triangle.v[2] * transformForPoints, color };
			toDraw++;

			if (toDraw >= DYNAMIC_TRIANGLES_SIZE) {
				gfx.ctx->Unmap(dynamicTriangles.Get(), 0);
				gfx.ctx->Draw(toDraw, 0);
				toDraw = 0;
				gfx.ctx->Map(dynamicTriangles.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
				data = reinterpret_cast<PcVert*>(resource.pData);
			}
		}
		gfx.ctx->Unmap(dynamicTriangles.Get(), 0);
		if (toDraw != 0) {
			gfx.ctx->Draw(toDraw, 0);
		}
	}

	debugShapesFragmentShaderConstantBuffer.lineWidth = 0.003f * 1920.0f / windowSize.x;
	debugShapesFragmentShaderConstantBuffer.smoothingWidth = debugShapesFragmentShaderConstantBuffer.lineWidth * (2.0f / 3.0f);
	gfx.updateConstantBuffer(debugShapesFragmentShaderConstantBufferResource, &debugShapesFragmentShaderConstantBuffer, sizeof(debugShapesFragmentShaderConstantBuffer));
	gfx.ctx->PSSetConstantBuffers(0, 1, debugShapesFragmentShaderConstantBufferResource.GetAddressOf());

	// checkDraw can be called last in loops because zero sized arrays are not allowed.
	// remember to put a draw or checkDraw after a loop.
		// Circle
	// !!!!!!!!!!!!! TODO: Make a templated class or a macro for dealing with drawing debug shapes. Later specifying the width might be needed.
	gfx.ctx->IASetIndexBuffer(fullscreenQuadIb.Get(), DXGI_FORMAT_R16_UINT, 0);
	gfx.ctx->IASetInputLayout(ptLayout.Get());
	{
		const UINT stride{ sizeof(PtVert) }, offset{ 0 };
		gfx.ctx->IASetVertexBuffers(0, 1, fullscreenQuadPtVb.GetAddressOf(), &stride, &offset);
	}

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
				.transform = makeTransform(circle.pos, orientation, Vec2{ circle.radius }),
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
				.transform = makeTransform(circle.pos, 0.0f, Vec2{ circle.radius }),
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
				.transform = makeTransform(point.pos, 0.0f, Vec2{ point.radius / camera.zoom }),
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
				.transform = makeTransform(hollowCircle.pos, 0.0f, Vec2{ hollowCircle.radius }),
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
				.transform = makeTransform(start, orientation, Vec2{ length + extraLength }),
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

auto Dx11Renderer::drawDynamicTexture(Vec2 pos, float height, DynamicTexture& dynamicTexture, bool interpolate) -> void {
	dynamicTexturesToDraw.push_back(DynamicTextureToDraw{
		.texture = &dynamicTexture,
		.pos = pos,
		.height = height,
		.interpolate = interpolate
	});
}

auto Dx11Renderer::createDynamicTextureData(Vec2T<i64> size) -> u64 {
	DynamicTextureData data{ gfx, size };
	const auto handle = reinterpret_cast<u64>(data.resourceView.Get());
	handleToDynamicTextureData[handle] = std::move(data);
	return handle;
}

auto Dx11Renderer::destroyDynamicTextureData(u64 handle) -> void {
	handleToDynamicTextureData.erase(handle);
}

auto Dx11Renderer::screenshot() -> ImageRgba {
	// https://stackoverflow.com/questions/21202215/c-directx11-capture-screen-and-save-to-file
	// TODO: Don't need to do as much if the texture is non multisampled.

	// Multisampled textures have to be resolved, which makes them non multisampled before they can be accessed on the CPU. If a texture is non multisampled it can be copied to a CPU accessible texture using CopyResource.

	ComPtr<ID3D11Texture2D> screenTexture;
	CHECK_WIN_HRESULT(gfx.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(screenTexture.GetAddressOf())));

	D3D11_TEXTURE2D_DESC desc;
	screenTexture->GetDesc(&desc);

	UINT support = 0;
	CHECK_WIN_HRESULT(gfx.device->CheckFormatSupport(desc.Format, &support));
	ASSERT(support & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RESOLVE);

	ComPtr<ID3D11Texture2D> nonMultisampledCopy;
	desc.SampleDesc.Count = 1;
	CHECK_WIN_HRESULT(gfx.device->CreateTexture2D(&desc, nullptr, nonMultisampledCopy.GetAddressOf()));

	for (UINT item = 0; item < desc.ArraySize; item++) {
		for (UINT level = 0; level < desc.MipLevels; level++) {
			UINT index = D3D11CalcSubresource(level, item, desc.MipLevels);
			gfx.ctx->ResolveSubresource(nonMultisampledCopy.Get(), index, screenTexture.Get(), index, desc.Format);
		}
	}

	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_STAGING;
		
	ComPtr<ID3D11Texture2D> cpuAccessibleCopy;
	CHECK_WIN_HRESULT(gfx.device->CreateTexture2D(&desc, NULL, cpuAccessibleCopy.GetAddressOf()));

	gfx.ctx->CopyResource(cpuAccessibleCopy.Get(), nonMultisampledCopy.Get());

	// Could probably just render to a texture to convert the formats.
	// Copy resource can automatically convert between some types, but only the ones described here https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#format-conversion-using-direct3d-101.
	if (desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM) {
		ASSERT_NOT_REACHED();
	}

	D3D11_MAPPED_SUBRESOURCE map;
	gfx.ctx->Map(cpuAccessibleCopy.Get(), 0, D3D11_MAP_READ, 0, &map);
	ImageRgba result{ Vec2T<i64>{ desc.Width, desc.Height } };
	for (i64 y = 0; y < result.size().y; y++) {
		const auto srcRow = reinterpret_cast<u8*>(map.pData) + y * map.RowPitch;
		const auto dstRowSize = sizeof(PixelRgba) * result.size().x;
		const auto dstRow = reinterpret_cast<u8*>(result.data()) + y * dstRowSize;
		memcpy(dstRow, srcRow, dstRowSize);
	}
	gfx.ctx->Unmap(cpuAccessibleCopy.Get(), 0);

	if (desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM) {
		for (auto& p : result) {
			std::swap(p.r, p.b);
		}
	}

	return result;
}

Dx11Renderer::DynamicTextureData::DynamicTextureData(Gfx& gfx, Vec2T<i64> size) {
	const D3D11_TEXTURE2D_DESC textureDesc{
		.Width = static_cast<UINT>(size.x),
		.Height = static_cast<UINT>(size.y),
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
}