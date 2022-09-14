#include <engine/time.hpp>
#include <engine/input.hpp>
#include <chrono>

auto Time::update(float frameDeltatime) -> void {
	currentFrameDeltaTime = frameDeltatime;

	if (Input::isKeyHeld(Keycode::T)) {
		// TODO: Print debug message about the currently set scale.
		if (Input::isKeyDown(Keycode::Plus)) {
			timeScale *= 2.0f;
		} else if (Input::isKeyDown(Keycode::Minus)) {
			timeScale *= 0.5f;
		}
	}
}

float Time::timeScale = 1.0f;
float Time::currentFrameDeltaTime;
