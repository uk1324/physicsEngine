#pragma once

#include <gfx/gfx.hpp>
#include <gfx/hlslTypes.hpp>

class Renderer {
public:
	Renderer(Gfx& gfx);
	auto update(Gfx& gfx) -> void;

private:
	struct PtVert {
		Vec2 pos;
		Vec2 texturePos;
	};
	ComPtr<ID3D11InputLayout> ptLayout;

	static constexpr PtVert fullscreenQuadVerts[]{
		{ Vec2{ -1.0f, 1.0f }, Vec2{ 0.0f, 0.0f } },
		{ Vec2{ 1.0f, 1.0f }, Vec2{ 0.0f, 1.0f } },
		{ Vec2{ -1.0f, -1.0f }, Vec2{ 1.0f, 0.0f } },
		{ Vec2{ 1.0f, -1.0f }, Vec2{ 1.0f, 1.0f } },
	};
	static constexpr u16 fullscreenQuadIndices[]{
		0, 1, 2, 2, 1, 3
	};
	ComPtr<ID3D11Buffer> fullscreenQuadPtVb;
	ComPtr<ID3D11Buffer> fullscreenQuadIb;

	ComPtr<ID3D11VertexShader> transformQuadPtShader;

	ComPtr<ID3D11PixelShader> circleShader;

	struct CircleInstance {
		float3x2 transform;
		float3 color;
	};
	ComPtr<ID3D11Buffer> circleShaderConstantBuffer;
	struct CircleShaderConstantBuffer {
		// Can use designed initializers {[0 ... <n>] = <value>};
		CircleInstance instanceData[100];
	};
};