#include <game/gameMain.hpp>
#include <engine/input.hpp>

auto GameMain::update() -> void {
	demos.update();
	return;

	//if (Input::isKeyDown(Keycode::TAB)) {
	//	if (currentScene == Scene::EDITOR) {
	//		editor.saveCurrentLevel();
	//		game.loadLevel();
	//		currentScene = Scene::GAME;
	//	} else if (currentScene == Scene::GAME) {
	//		//editor.registerInputButtons();
	//		currentScene = Scene::EDITOR;
	//	}
	//}

	//if (currentScene == Scene::EDITOR) {
	//	editor.update();
	//} else if (currentScene == Scene::GAME) {
	//	game.update();
	//}
}
