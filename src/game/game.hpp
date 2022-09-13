#pragma once

#include <game/renderer.hpp>

class Game {
public:
	Game(Gfx& gfx);

	auto update(Gfx& gfx) -> void;

	Renderer renderer;
};