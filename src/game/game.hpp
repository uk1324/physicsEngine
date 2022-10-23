#pragma once

#include <game/renderer.hpp>
#include <game/entities.hpp>

class Game {
public:
	Game(Gfx& gfx);

	auto update(Gfx& gfx) -> void;

	auto integrate(Transform& transform, PhysicsInfo& physics) const -> void;

	auto circlesScene() -> void;
	auto rollingSphereScene() -> void;
	auto lineAndCircleScene() -> void;
	auto twoConvexPolygonsScene() -> void;

	Vec2 cameraPos{ 0.0f };
	float cameraZoom{ 1.0f };

	Vec2* controlledVel{ nullptr };
	Vec2* followedPos{ nullptr };

	Renderer renderer;

	float gravityAcceleration{ 0.0f };

	PhysicsMaterial material0;
	PhysicsMaterial material1;
};