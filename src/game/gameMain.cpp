#include <game/gameMain.hpp>
#include <engine/input.hpp>

GameMain::GameMain(Gfx& gfx) 
	: renderer{ gfx }
	, gfx{ gfx }
	, pixelGames{ gfx } {}

auto GameMain::update() -> void {
	pixelGames.update(gfx, renderer);
	return;

	if (Input::isKeyDown(Keycode::TAB)) {
		if (currentScene == Scene::EDITOR) {
			editor.saveCurrentLevel();
			game.loadLevel();
			currentScene = Scene::GAME;
		} else if (currentScene == Scene::GAME) {
			//editor.registerInputButtons();
			currentScene = Scene::EDITOR;
		}
	}

	if (currentScene == Scene::EDITOR) {
		editor.update(gfx, renderer);
	} else if (currentScene == Scene::GAME) {
		game.update(gfx, renderer);
	}
}
