#pragma once

#include <math/vec2.hpp>

#include <bitset>
#include <unordered_map>

enum class Keycode : u8 {
	// Windows virtual key codes. Not using the defines in windows.h so it doesn't need to be included in every file using Input.
	TAB = 0x09,
	SHIFT = 0x10,
	CTRL = 0x11,
	LEFT = 0x25,
	UP = 0x26,
	RIGHT = 0x27,
	DOWN = 0x28,
	DEL = 0x2E,
	A = 'A',
	B = 'B',
	C = 'C',
	D = 'D',
	F = 'F',
	G = 'G',
	H = 'H',
	I = 'I',
	J = 'J',
	K = 'K',
	O = 'O',
	P = 'P',
	Q = 'Q',
	R = 'R',
	S = 'S',
	T = 'T',
	U = 'U',
	V = 'V',
	W = 'W',
	X = 'X',
	Y = 'Y',
	Z = 'Z',
	F3 = 0x72,
	Plus = 0xBB,
	Minus = 0xBD,
	COUNT
};

enum class MouseButton : u8 {
	// Mouse buttons are also virtual key codes, but they are not sent by WM_KEY<action> events.
	LEFT = 0x01,
	RIGHT = 0x02,
	MIDDLE = 0x04,
	COUNT,
};

// There is no way to make private variables without a class if templated functions are used and also the on<action> functions couldn't be private.
class Input {
public:
	template<typename ButtonEnum>
	static auto registerKeyButton(Keycode key, ButtonEnum button) -> void;
	template<typename ButtonEnum>
	static auto registerMouseButton(MouseButton mouseButton, ButtonEnum button) -> void;

	template<typename ButtonEnum> 
	static auto isButtonDown(ButtonEnum button) -> bool;
	template<typename ButtonEnum>
	static auto isButtonDownWithAutoRepeat(ButtonEnum button) -> bool;
	template<typename ButtonEnum> 
	static auto isButtonUp(ButtonEnum button) -> bool;
	template<typename ButtonEnum> 
	static auto isButtonHeld(ButtonEnum button) -> bool;

	static auto isKeyDown(Keycode key) -> bool;
	static auto isKeyDownWithAutoRepeat(Keycode key) -> bool;
	static auto isKeyUp(Keycode key) -> bool;
	static auto isKeyHeld(Keycode key) -> bool;

	static auto isMouseButtonDown(MouseButton button) -> bool;
	static auto isMouseButtonUp(MouseButton button) -> bool;
	static auto isMouseButtonHeld(MouseButton button) -> bool;

	static auto windowSpaceToScreenSpace(Vec2 v) -> Vec2;
	static auto cursorPos() -> Vec2 { return cursorPos_; };
	static auto cursorPosWindowSpace() -> Vec2 { return cursorPosWindowSpace_; };
	static auto scrollDelta() -> float;
	static auto anyKeyPressed() -> bool;

	static auto update() -> void;

	static bool ignoreImGuiWantCapture;

	static auto onKeyDown(u8 virtualKeyCode, bool autoRepeat) -> void;
	static auto onKeyUp(u8 virtualKeyCode) -> void;
	static auto onMouseMove(Vec2 mousePos) -> void;
	static auto onMouseScroll(i16 scroll) -> void;
private:

	static constexpr auto MOUSE_BUTTON_COUNT = static_cast<size_t>(MouseButton::COUNT);
	static constexpr auto KEYCODE_COUNT = static_cast<size_t>(Keycode::COUNT);
	// Virtual keycodes are always one byte so this is kind of pointless.
	static constexpr auto VIRTUAL_KEY_COUNT = (MOUSE_BUTTON_COUNT > KEYCODE_COUNT) ? MOUSE_BUTTON_COUNT : KEYCODE_COUNT;
	static std::bitset<VIRTUAL_KEY_COUNT> keyDown;
	static std::bitset<VIRTUAL_KEY_COUNT> keyDownWithAutoRepeat;
	// KeyUp doesn't get auto repeated.
	static std::bitset<VIRTUAL_KEY_COUNT> keyUp;
	static std::bitset<VIRTUAL_KEY_COUNT> keyHeld;

	static std::unordered_multimap<u8, int> virtualKeyToButton;
	static std::unordered_map<int, bool> buttonDown;
	static std::unordered_map<int, bool> buttonDownWithAutoRepeat;
	static std::unordered_map<int, bool> buttonUp;
	static std::unordered_map<int, bool> buttonHeld;

	static Vec2 cursorPos_;
	static Vec2 cursorPosWindowSpace_;
	static float scrollDelta_;
	static bool anyKeyPressed_;
};

template<typename ButtonEnum>
auto Input::registerKeyButton(Keycode key, ButtonEnum button) -> void {
	const auto code = static_cast<int>(button);
	virtualKeyToButton.insert({ static_cast<u8>(key), code });
	buttonDown[code] = false;
	buttonDownWithAutoRepeat[code] = false;
	buttonUp[code] = false;
	buttonHeld[code] = false;
}

template<typename ButtonEnum>
auto Input::registerMouseButton(MouseButton mouseButton, ButtonEnum button) -> void {
	registerKeyButton(static_cast<Keycode>(mouseButton), button);
}

// Check to prevent accidentally using wrong enums. Not using concepts because they still aren't fully suported. And for some reason can't be created in class scope.
#define CHECK_BUTTON_ENUM() static_assert(!std::is_same_v<ButtonEnum, Keycode> && !std::is_same_v<ButtonEnum, MouseButton>);

template<typename ButtonEnum>
auto Input::isButtonDown(ButtonEnum button) -> bool {
	CHECK_BUTTON_ENUM();
	return buttonDown[static_cast<int>(button)];
}

template<typename ButtonEnum>
auto Input::isButtonDownWithAutoRepeat(ButtonEnum button) -> bool {
	CHECK_BUTTON_ENUM();
	return buttonDownWithAutoRepeat[static_cast<int>(button)];
}

template<typename ButtonEnum>
auto Input::isButtonUp(ButtonEnum button) -> bool {
	CHECK_BUTTON_ENUM();
	return buttonUp[static_cast<int>(button)];
}

template<typename ButtonEnum>
auto Input::isButtonHeld(ButtonEnum button) -> bool {
	CHECK_BUTTON_ENUM();
	return buttonHeld[static_cast<int>(button)];
}

#undef CHECK_BUTTON_ENUM