#pragma once

#include <game/renderer.hpp>

class Game2 {
public:
	Game2(Gfx& gfx);

	auto update(Gfx& gfx) -> void;

	Vec2* controlledValue = nullptr;
	Vec2* followedPos = nullptr;

	Vec2 cameraPos{ 0.0f };
	float cameraZoom = 1.0f;

	Vec2 gravity{ 0.0f };

	static bool warmStarting;
	static bool positionCorrection;
	static bool accumulateImpulses;

	Renderer renderer;
};