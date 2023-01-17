#pragma once

#include <game/game.hpp>
#include <game/editor/editor.hpp>
#include <demos/demos.hpp>

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

	Demos pixelGames;
};