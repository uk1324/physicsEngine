#pragma once

#include <game/game.hpp>
#include <game/editor/editor.hpp>

struct GameMain {
	GameMain(Gfx& gfx);
	auto update() -> void;

	Gfx& gfx;
	Renderer renderer;

	Game game;
	Editor editor;
};