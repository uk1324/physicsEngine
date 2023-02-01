#pragma once

#include <game/game.hpp>
#include <demos/demos.hpp>

struct GameMain {
	auto update() -> void;

	Game game;
	Demos demos;
};