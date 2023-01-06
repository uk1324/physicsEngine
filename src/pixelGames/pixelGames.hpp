#pragma once

#include <gfx/gfx.hpp>
#include <game/renderer.hpp>

#include <complex>

struct PixelGames {
	PixelGames(Gfx& gfx);

	auto update(Gfx& gfx, Renderer& renderer) -> void;

	float elapsed = 0.0f;
	bool doFourierTransform = false;
	DynamicTexture texture;
	std::vector<std::complex<float>> result;

	u8* image;
};