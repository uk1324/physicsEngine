#pragma once

#include <utils/int.hpp>
#include <math/vec2.hpp>

#include <bitset>
#include <unordered_map>

enum class Keycode : u8 {
	// Windows virtual key codes. Not using the defines in windows.h so it doesn't need to be included in every file using Input.
	LEFT = 0x25,
	UP = 0x26,
	RIGHT = 0x27,
	DOWN = 0x28,
	A = 'A',
	D = 'D',
	S = 'S',
	W = 'W',
	COUNT
};

class Window;

// There is no way to make private variables without a class if templated functions are used and also the on<action> functions couldn't be private.
class Input {
	friend class Window;

public:
	template<typename ButtonEnum>
	static auto registerKeyButton(Keycode key, ButtonEnum button) -> void;

	template<typename ButtonEnum> 
	static auto isButtonDown(ButtonEnum button) -> bool;
	template<typename ButtonEnum> 
	static auto isButtonUp(ButtonEnum button) -> bool;
	template<typename ButtonEnum> 
	static auto isButtonHeld(ButtonEnum button) -> bool;

	static auto isKeyDown(Keycode key) -> bool;
	static auto isKeyUp(Keycode key) -> bool;
	static auto isKeyHeld(Keycode key) -> bool;

	static auto cursorPos() -> Vec2 { return cursorPos_; };

	static auto update() -> void;

private:
	static auto onKeyDown(u64 wParam, u64 lParam) -> void;
	static auto onKeyUp(u64 wParam, u64 lParam) -> void;
	static auto onMouseMove(Vec2 mousePos) -> void;

	static constexpr auto KEY_COUNT = static_cast<size_t>(Keycode::COUNT);
	static std::bitset<KEY_COUNT> keyDown;
	static std::bitset<KEY_COUNT> keyUp;
	static std::bitset<KEY_COUNT> keyHeld;

	static std::unordered_multimap<u8, int> keycodeToButton;
	static std::unordered_map<int, bool> buttonDown;
	static std::unordered_map<int, bool> buttonUp;
	static std::unordered_map<int, bool> buttonHeld;

	static Vec2 cursorPos_;
};

template<typename ButtonEnum>
auto Input::registerKeyButton(Keycode key, ButtonEnum button) -> void {
	const auto code = static_cast<int>(button);
	keycodeToButton.insert({ static_cast<u8>(key), code });
	buttonDown[code] = false;
	buttonUp[code] = false;
	buttonHeld[code] = false;
}

template<typename ButtonEnum>
auto Input::isButtonDown(ButtonEnum button) -> bool {
	return buttonDown[static_cast<int>(button)];
}

template<typename ButtonEnum>
auto Input::isButtonUp(ButtonEnum button) -> bool {
	return buttonUp[static_cast<int>(button)];
}

template<typename ButtonEnum>
auto Input::isButtonHeld(ButtonEnum button) -> bool {
	return buttonHeld[static_cast<int>(button)];
}
