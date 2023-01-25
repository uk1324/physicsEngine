#pragma once

#include <game/game.hpp>
#include <game/editor/editor.hpp>
#include <demos/demos.hpp>

struct GameMain {
	auto update() -> void;

	Game game;
	Editor editor;

	enum class Scene {
		GAME,
		EDITOR,
	};
	Scene currentScene = Scene::GAME;

	Demos pixelGames;
};