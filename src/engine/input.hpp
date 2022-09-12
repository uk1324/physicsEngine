#pragma once

#include <utils/int.hpp>

#include <bitset>

enum class Keycode {
	// Windows virtual key codes. Not using the defines in windows.h so it doesn't need to be included in every file using Input.
	A = 'A',
	D = 'D',
	S = 'S',
	W = 'W',
	COUNT
};

class Window;

class Input {
	friend class Window;

public:
	static auto isKeyDown(Keycode key) -> bool;
	static auto isKeyUp(Keycode key) -> bool;
	static auto isKeyHeld(Keycode key) -> bool;

	static auto update() -> void;

private:
	static auto onKeyDown(u64 wParam, u64 lParam) -> void;
	static auto onKeyUp(u64 wParam, u64 lParam) -> void;

	static constexpr auto KEY_COUNT = static_cast<size_t>(Keycode::COUNT);
	static std::bitset<KEY_COUNT> keyDown;
	static std::bitset<KEY_COUNT> keyUp;
	static std::bitset<KEY_COUNT> keyHeld;
};