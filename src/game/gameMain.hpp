#pragma once

#include <game/game.hpp>
#include <game/editor/editor.hpp>
#include <pixelGames/pixelGames.hpp>

struct GameMain {
	GameMain(Gfx& gfx);
	auto update() -> void;

	Gfx& gfx;
	Renderer renderer;

	Game game;
	Editor editor;

	enum class Scene {
		GAME,
		EDITOR,
	};
	Scene currentScene = Scene::EDITOR;

	PixelGames pixelGames;
};