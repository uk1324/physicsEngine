#pragma once

#include <gfx/gfx.hpp>
#include <game/renderer.hpp>

struct PixelGames {
	PixelGames(Gfx& gfx);

	auto update(Gfx& gfx, Renderer& renderer) -> void;

	float elapsed = 0.0f;
	DynamicTexture texture;
};