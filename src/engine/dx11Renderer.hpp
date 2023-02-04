#include <gfx/gfx.hpp>
#include <gfx/hlslTypes.hpp>
#include <engine/renderer.hpp>

#include <vector>
#include <unordered_map>

#pragma warning(push)
#pragma warning(disable : 4324) // Padding added.
class Dx11Renderer {
public:
	Dx11Renderer(Gfx& gfx);
	auto update(Camera& camera, std::optional<Vec2> windowSizeIfRenderingToTexture = std::nullopt) -> void;
	auto drawDynamicTexture(Vec2 pos, float height, DynamicTexture& dynamicTexture, bool interpolate = false) -> void;

	auto createDynamicTextureData(Vec2T<i64> size) -> u64;
	auto destroyDynamicTextureData(u64 handle) -> void;
	auto screenshot() -> ImageRgba;

	ComPtr<ID3D11ShaderResourceView> windowTextureShaderResourceView;
	static constexpr Vec2 textureSize{ 1920.0f, 1080.0f };
private:
	ComPtr<ID3D11Texture2D> windowTexture;
	ComPtr<ID3D11RenderTargetView> windowTextureRenderTargetView;

	ComPtr<ID3D11SamplerState> nearestNeighbourTextureSamplerState;
	ComPtr<ID3D11SamplerState> linearTextureSamplerState;

	struct DynamicTextureData {
		DynamicTextureData() {};
		DynamicTextureData(Gfx& gfx, Vec2T<i64> size);
		ComPtr<ID3D11Texture2D> texture;
		ComPtr<ID3D11ShaderResourceView> resourceView;
	};

	std::unordered_map<u64, DynamicTextureData> handleToDynamicTextureData;

	struct DynamicTextureToDraw {
		DynamicTexture* texture;
		Vec2 pos;
		float height;
		bool interpolate;
	};
	std::vector<DynamicTextureToDraw> dynamicTexturesToDraw;

	Gfx& gfx;

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

	struct PcVert {
		Vec2 pos;
		Vec3 color;
	};
	ComPtr<ID3D11InputLayout> pcLayout;
	static constexpr usize DYNAMIC_TRIANGLES_SIZE = 3 * 100;
	ComPtr<ID3D11Buffer> dynamicTriangles;

	VertexShader vsColoredTriangle;
	ComPtr<ID3D11PixelShader> psColoredTriangle;

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