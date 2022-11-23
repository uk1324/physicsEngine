#pragma once

#include <game/renderer.hpp>
#include <game/bvhCollisionSystem.hpp>

// TODO: Replay system. Mouse position would need to be either saved in world space or later transformed by the camera transform. Could store 2 camera transforms one for the actual camera and the replay camera.

class Game {
public:
	Game(Gfx& gfx);

	auto detectCollisions() -> void;
	auto drawUi() -> void;
	auto update(Gfx& gfx) -> void;

	Vec2* controlledValue = nullptr;
	Vec2* followedPos = nullptr;

	Camera camera;

	Vec2 gravity{ 0.0f };
	float angularDamping = 0.98f;

	bool updatePhysics = true;
	bool cameraFollow = true;
	bool drawContacts = false;

	bool drawTrajectory = false;
	Vec2 initialVelocity{ 1.0f };

	static bool warmStarting;
	static bool positionCorrection;
	static bool accumulateImpulses;

	BvhCollisionSystem collisionSystem;
	Renderer renderer;
};