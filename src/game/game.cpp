#include <game/game.hpp>
#include <game/input.hpp>

Game::Game(Gfx& gfx)
	: renderer(gfx) {
	Input::registerKeyButton(Keycode::W, GameButton::UP);
	Input::registerKeyButton(Keycode::S, GameButton::DOWN);
	Input::registerKeyButton(Keycode::A, GameButton::LEFT);
	Input::registerKeyButton(Keycode::D, GameButton::RIGHT);

	Input::registerKeyButton(Keycode::UP, GameButton::UP);
	Input::registerKeyButton(Keycode::DOWN, GameButton::DOWN);
	Input::registerKeyButton(Keycode::LEFT, GameButton::LEFT);
	Input::registerKeyButton(Keycode::RIGHT, GameButton::RIGHT);
}

auto Game::update(Gfx& gfx) -> void {
	renderer.update(gfx);
}
