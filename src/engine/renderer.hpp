#pragma once

#include <utils/imageRgba.hpp>
#include <engine/camera.hpp>

struct DynamicTexture : ImageRgba {
	DynamicTexture(Vec2T<i64> size);
	~DynamicTexture();
	u64 textureHandle;
};

class Renderer {
	friend struct DynamicTexture;

public:
	static auto init() -> void;
	static auto terminate() -> void;
	static auto update(Camera& camera, std::optional<Vec2> windowSizeIfRenderingToTexture = std::nullopt) -> void;
	static auto updateFrameStart() -> void;
	static auto updateFrameEnd() -> void;

	static auto drawDynamicTexture(Vec2 pos, float height, DynamicTexture& dynamicTexture, bool interpolate = false) -> void;
	static auto outputTextureHandle() -> void*;
	static auto textureSize() -> Vec2;
private:

	static auto createDynamicTexture(Vec2T<i64> size) -> u64;
	static auto destroyDynamicTexture(u64 textureHandle);
};