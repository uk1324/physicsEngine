#pragma once

#include <game/renderer.hpp>
#include <game/entities.hpp>

class Game {
public:
	Game(Gfx& gfx);

	auto update(Gfx& gfx) -> void;

	Renderer renderer;

	PhysicsMaterial material0;
	PhysicsMaterial material1;
};