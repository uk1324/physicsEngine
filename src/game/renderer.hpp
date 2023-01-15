#pragma once

#include <gfx/gfx.hpp>
#include <gfx/hlslTypes.hpp>
#include <utils/imageRgba.hpp>
#include <game/camera.hpp>

#include <vector>

// TODO: Maybe use order independent transparency. It shouldn't be expensive because it is just quads.
// TODO: Shader preprocesor that when it find DEBUG_OUT(<variable>) it replaces it with if (<variable_flag>) return <variable> and creates a variable <variable_flag>. It also creates maybe a json struct describing the flags. It uses the struct to display a select menu using ImGui. One limitation that it can only be used in the main function.

struct DynamicTexture : ImageRgba {
	ComPtr<ID3D11Texture2D> texture;
	ComPtr<ID3D11ShaderResourceView> resourceView;

	DynamicTexture(Gfx& gfx, Vec2T<i64> size);
};

#pragma warning(push)
#pragma warning(disable : 4324) // Padding added.
class Renderer {
public:
	Renderer(Gfx& gfx);
	auto update(Gfx& gfx, const Camera& camera, Vec2 windowSize, bool renderToTexture) -> void;
	auto drawDynamicTexture(Vec2 pos, float size, DynamicTexture& dynamicTexture) -> void;

	static constexpr Vec2 textureSize{ 1920.0f, 1080.0f };
	ComPtr<ID3D11Texture2D> windowTexture;
	ComPtr<ID3D11RenderTargetView> windowTextureRenderTargetView;
	ComPtr<ID3D11ShaderResourceView> windowTextureShaderResourceView;

	ComPtr<ID3D11SamplerState> pixelTextureSamplerState;
private:
	struct DynamicTextureToDraw {
		DynamicTexture* texture;
		Vec2 pos;
		float size;
	};
	std::vector<DynamicTextureToDraw> dynamicTexturesToDraw;

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

	VertexShader vsTexturedQuad;
	ComPtr<ID3D11PixelShader> psTexturedQuad;
	ComPtr<ID3D11Buffer> texturedQuadConstantBufferResource;
	struct TexturedQuadConstantBuffer {
		float3x2 transform;
	};
	TexturedQuadConstantBuffer texturedQuadConstantBuffer;

	// TODO: Screen space derivatives of the quads (ddx and ddy) could be used to allow non uniform scaling easily.
	VertexShader vsCircle;
	ComPtr<ID3D11PixelShader> psCircleCollider;
	ComPtr<ID3D11PixelShader> psCircle;
	ComPtr<ID3D11PixelShader> psHollowCircle;
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
#pragma warning(pop)