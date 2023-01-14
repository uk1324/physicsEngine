#pragma once

#include <gfx/gfx.hpp>
#include <game/renderer.hpp>
#include <pixelGames/fourierTransformDemo.hpp>

#include <complex>

struct PixelGames {
	PixelGames(Gfx& gfx);
	auto update(Gfx& gfx, Renderer& renderer) -> void;

	FourierTransformDemo fourierTransformDemo;
};