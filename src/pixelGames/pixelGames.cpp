#include <pixelGames/pixelGames.hpp>

PixelGames::PixelGames(Gfx& gfx)
	: fourierTransformDemo{ gfx } {}


auto PixelGames::update(Gfx& gfx, Renderer& renderer) -> void {
	fourierTransformDemo.update(gfx, renderer);
}
