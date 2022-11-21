#pragma once

#include <gfx/gfx.hpp>
#include <gfx/hlslTypes.hpp>

struct Camera {
	Camera(Vec2 pos = Vec2{ 0.0f }, float zoom = 1.0f);

	auto interpolateTo(Vec2 desiredPos, float speed) -> void;
	auto cameraTransform() const -> Mat3x2;
	auto screenSpaceToCameraSpace(Vec2 screenSpacePos) -> Vec2;

	Vec2 pos;
	float zoom;
	float aspectRatio = 1.0f;
};

// TODO: Maybe use order independent transparency. It shouldn't be expensive because it is just quads.
class Renderer {
public:
	Renderer(Gfx& gfx);
	auto update(Gfx& gfx, const Camera& camera) -> void;

private:

	struct PtVert {
		Vec2 pos;
		Vec2 texturePos;
	};
	ComPtr<ID3D11InputLayout> ptLayout;

	static constexpr PtVert fullscreenQuadVerts[]{
		{ Vec2{ -1.0f, 1.0f }, Vec2{ 0.0f, 0.0f } },
		{ Vec2{ 1.0f, 1.0f }, Vec2{ 1.0f, 0.0f } },
		{ Vec2{ -1.0f, -1.0f }, Vec2{ 0.0f, 1.0f } },
		{ Vec2{ 1.0f, -1.0f }, Vec2{ 1.0f, 1.0f } },
	};
	static constexpr u16 fullscreenQuadIndices[]{
		0, 1, 2, 2, 1, 3
	};
	ComPtr<ID3D11Buffer> fullscreenQuadPtVb;
	ComPtr<ID3D11Buffer> fullscreenQuadIb;

	// TODO: Screen space derivatives of the quads (ddx and ddy) could be used to allow non uniform scaling easily.
	VertexShader vsCircle;
	ComPtr<ID3D11PixelShader> psCircleCollider;
	ComPtr<ID3D11PixelShader> psCircle;
	struct CircleInstance {
		float invRadius;
		float3x2 transform; // Scale has to be uniform.
		float3 color;
	};
	ComPtr<ID3D11Buffer> circleShaderConstantBufferResource;
	struct CircleConstantBuffer {
		CircleInstance instanceData[100];
	};
	CircleConstantBuffer circleShaderConstantBuffer;

	VertexShader vsLine;
	ComPtr<ID3D11PixelShader> psLine;
	struct LineInstance {
		float invScale;
		float3x2 transform; // Scale has to be uniform.
		float3 color;
	};
	ComPtr<ID3D11Buffer> lineShaderConstantBufferResource;
	struct LineShaderConstantBuffer {
		LineInstance instanceData[100];
	};
	LineShaderConstantBuffer lineShaderConstantBuffer;

	VertexShader vsParabola;
	ComPtr<ID3D11PixelShader> psParabola;
	struct ParabolaInstance {
		float invScale;
		float3x2 transform; // Scale has to be uniform.
		float3 color;
		float3 cofefficients;
	};
	ComPtr<ID3D11Buffer> parabolaShaderConstantBufferResource;
	struct ParabolaShaderConstantBuffer {
		ParabolaInstance instanceData[100];
	};
	ParabolaShaderConstantBuffer parabolaShaderConstantBuffer;

	ComPtr<ID3D11Buffer> debugShapesFragmentShaderConstantBufferResource;
	struct alignas(16) /* min constant buffer size */ DebugShapesFragmentShaderConstantBuffer {
		float lineWidth;
		float smoothingWidth;
	};
	DebugShapesFragmentShaderConstantBuffer debugShapesFragmentShaderConstantBuffer;
};