#include <engine/time.hpp>
#include <engine/input.hpp>
#include <utils/io.hpp>

#include <chrono>

auto Time::update(float frameDeltatime) -> void {
	currentFrameDeltaTime = frameDeltatime;

	if (Input::isKeyHeld(Keycode::T)) {
		const auto old = timeScale;
		if (Input::isKeyDown(Keycode::Plus))
			timeScale *= 2.0f;
		else if (Input::isKeyDown(Keycode::Minus))
			timeScale *= 0.5f;

		if (timeScale != old)
			LOG_INFO("time scale set to %g", timeScale);
	}
}

float Time::timeScale = 1.0f;
float Time::currentFrameDeltaTime;
