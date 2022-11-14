#pragma once

#include <game/renderer.hpp>
#include <game/bvhCollisionSystem.hpp>

class Game2 {
public:
	Game2(Gfx& gfx);

	auto detectCollisions() -> void;
	auto update(Gfx& gfx) -> void;

	Vec2* controlledValue = nullptr;
	Vec2* followedPos = nullptr;

	Camera camera;

	Vec2 gravity{ 0.0f };

	static bool warmStarting;
	static bool positionCorrection;
	static bool accumulateImpulses;

	BvhCollisionSystem collisionSystem;
	Renderer renderer;
};