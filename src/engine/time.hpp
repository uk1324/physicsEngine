#pragma once

class Time {
public:
	static auto deltaTime() -> float { return currentFrameDeltaTime; };

	static auto update(float currentFrameDeltaTime) -> void;

private:
	static float currentFrameDeltaTime;
};