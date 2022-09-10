#pragma once

#include <game/renderer.hpp>

class Game {
public:
	auto update(Gfx& gfx) -> void;

	Renderer renderer;
};