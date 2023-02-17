#pragma once

#include <utils/imageRgba.hpp>
#include <engine/camera.hpp>

struct DynamicTexture : ImageRgba {
	DynamicTexture(Vec2T<i64> size);
	DynamicTexture(const DynamicTexture&) = delete;
	auto operator=(const DynamicTexture&) -> void = delete;
	DynamicTexture(DynamicTexture&& other) noexcept;
	auto operator=(DynamicTexture&& other) noexcept;
	~DynamicTexture();
	u64 textureHandle;
};

class Renderer {
	friend struct DynamicTexture;

public:
	static auto init() -> void;
	static auto terminate() -> void;
	static auto update(Camera& camera, std::optional<float> gridSmallCellSize = std::nullopt, std::optional<Vec2> windowSizeIfRenderingToTexture = std::nullopt) -> void;
	static auto updateFrameStart() -> void;
	static auto updateFrameEnd(bool enableVsync) -> void;
	static auto screenshot() -> ImageRgba;

	static auto drawDynamicTexture(Vec2 pos, float height, DynamicTexture& dynamicTexture, bool interpolate = false) -> void;
	static auto outputTextureHandle() -> void*;
	static auto textureSize() -> Vec2;
private:

	static auto createDynamicTexture(Vec2T<i64> size) -> u64;
	static auto destroyDynamicTexture(u64 textureHandle);
};