#include <engine/input.hpp>

auto Input::isKeyDown(Keycode key) -> bool {
	return keyDown[static_cast<size_t>(key)];
}

auto Input::isKeyUp(Keycode key) -> bool {
	return keyUp[static_cast<size_t>(key)];
}

auto Input::isKeyHeld(Keycode key) -> bool {
	return keyHeld[static_cast<size_t>(key)];
}

auto Input::update() -> void {
	keyDown.reset();
	keyUp.reset();
}

auto Input::onKeyDown(u64 wParam, u64 lParam) -> void {
	const auto autoRepeat{ static_cast<bool>(lParam >> 30) };
	const auto& virtualKeyCode{ wParam };
	if (autoRepeat || virtualKeyCode > KEY_COUNT)
		return;
	
	keyDown.set(virtualKeyCode);
	keyHeld.set(virtualKeyCode);
}

auto Input::onKeyUp(u64 wParam, u64 lParam) -> void {
	const auto& virtualKeyCode{ wParam };
	keyUp.set(virtualKeyCode);
	keyHeld.set(virtualKeyCode, false);
}

std::bitset<Input::KEY_COUNT> Input::keyDown;
std::bitset<Input::KEY_COUNT> Input::keyUp;
std::bitset<Input::KEY_COUNT> Input::keyHeld;