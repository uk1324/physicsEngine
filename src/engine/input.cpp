#include <engine/input.hpp>
#include <engine/window.hpp>
#include <imgui/imgui.h>

auto Input::isKeyDown(Keycode key) -> bool {
	return keyDown[static_cast<size_t>(key)];
}

auto Input::isKeyDownWithAutoRepeat(Keycode key) -> bool {
	return keyDownWithAutoRepeat[static_cast<usize>(key)];
}

auto Input::isKeyUp(Keycode key) -> bool {
	return keyUp[static_cast<usize>(key)];
}

auto Input::isKeyHeld(Keycode key) -> bool {
	return keyHeld[static_cast<usize>(key)];
}

auto Input::isMouseButtonDown(MouseButton button) -> bool {
	return isKeyDown(static_cast<Keycode>(button));
}

auto Input::isMouseButtonUp(MouseButton button) -> bool {
	return isKeyUp(static_cast<Keycode>(button));
}

auto Input::isMouseButtonHeld(MouseButton button) -> bool {
	return isKeyHeld(static_cast<Keycode>(button));
}

auto Input::update() -> void {
	keyDown.reset();
	keyDownWithAutoRepeat.reset();
	keyUp.reset();

	for (auto& [_, isDown] : buttonDown) isDown = false;
	for (auto& [_, isDown] : buttonDownWithAutoRepeat) isDown = false;
	for (auto& [_, isUp] : buttonUp) isUp = false;

	scrollDelta_ = 0.0f;
}

static auto setIfAlreadyExists(std::unordered_map<int, bool>& map, int key, bool value) -> void {
	if (auto it = map.find(key); it != map.end())
		it->second = value;
}

static auto isMouseButton(u8 vkCode) -> bool {
	const auto code = static_cast<MouseButton>(vkCode);
	return code == MouseButton::LEFT || code == MouseButton::RIGHT || code == MouseButton::MIDDLE;
}

static auto isKeyboardKey(u8 vkCode) -> bool {
	return !isMouseButton(vkCode);
}

auto Input::onKeyDown(u8 virtualKeyCode, bool autoRepeat) -> void {
	if (virtualKeyCode >= VIRTUAL_KEY_COUNT)
		return;
	
	if (ImGui::GetIO().WantCaptureMouse && isMouseButton(virtualKeyCode))
		return;

	if (!autoRepeat) {
		keyDown.set(virtualKeyCode);
		keyHeld.set(virtualKeyCode);
	}
	keyDownWithAutoRepeat.set(virtualKeyCode);
	

	const auto buttons = virtualKeyToButton.equal_range(virtualKeyCode);
	for (auto button = buttons.first; button != buttons.second; ++button) {
		const auto& [_, buttonCode] = *button;
		if (!autoRepeat) {
			setIfAlreadyExists(buttonDown, buttonCode, true);
			setIfAlreadyExists(buttonHeld, buttonCode, true);
		}
		setIfAlreadyExists(buttonDownWithAutoRepeat, buttonCode, true);
	}
}

auto Input::onKeyUp(u8 virtualKeyCode) -> void {
	if (virtualKeyCode >= VIRTUAL_KEY_COUNT)
		return;

	if (ImGui::GetIO().WantCaptureMouse && isMouseButton(virtualKeyCode))
		return;

	keyUp.set(virtualKeyCode);
	keyHeld.set(virtualKeyCode, false);

	const auto buttons = virtualKeyToButton.equal_range(virtualKeyCode);
	for (auto button = buttons.first; button != buttons.second; ++button) {
		const auto& [_, buttonCode] = *button;
		setIfAlreadyExists(buttonUp, buttonCode, true);
		setIfAlreadyExists(buttonHeld, buttonCode, false);
	}
}

auto Input::onMouseMove(Vec2 mousePos) -> void {
	cursorPos_ = mousePos;
	cursorPos_ /= (Window::size() / 2.0f);
	cursorPos_.y = -cursorPos_.y;
	cursorPos_ += Vec2{ -1.0f, 1.0f };
}

auto Input::onMouseScroll(i16 scroll) -> void {
	if (ImGui::GetIO().WantCaptureMouse)
		return;

	scrollDelta_ = scroll / WHEEL_DELTA;
}

std::bitset<Input::VIRTUAL_KEY_COUNT> Input::keyDown;
std::bitset<Input::VIRTUAL_KEY_COUNT> Input::keyUp;
std::bitset<Input::VIRTUAL_KEY_COUNT> Input::keyDownWithAutoRepeat;
std::bitset<Input::VIRTUAL_KEY_COUNT> Input::keyHeld;

std::unordered_multimap<u8, int> Input::virtualKeyToButton;
std::unordered_map<int, bool> Input::buttonDown;
std::unordered_map<int, bool> Input::buttonDownWithAutoRepeat;
std::unordered_map<int, bool> Input::buttonUp;
std::unordered_map<int, bool> Input::buttonHeld;

Vec2 Input::cursorPos_;
float Input::scrollDelta_;