#pragma once

class Time {
public:
	static auto deltaTime() -> float { return currentFrameDeltaTime * timeScale; };

	static auto update(float currentFrameDeltaTime) -> void;

	// This can also be used to set the scale only for a specific function by just setting before calling the function and resetting to the old value after the call.
	static float timeScale;

private:
	static float currentFrameDeltaTime;
};