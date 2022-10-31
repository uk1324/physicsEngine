#pragma once

#include <game/renderer.hpp>
#include <game/entities.hpp>

class Game {
public:
	Game(Gfx& gfx);

	auto update(Gfx& gfx) -> void;
	auto handleInput() -> void;

	auto integrate(Transform& transform, PhysicsInfo& physics) const -> void;

	auto circlesScene2() -> void;
	auto circlesScene() -> void;
	auto rollingCircleScene() -> void;
	auto lineAndCircleScene() -> void;
	auto twoConvexPolygonsScene() -> void;

	Vec2 cameraPos{ 0.0f };
	float cameraZoom{ 1.0f };

	Vec2* controlledVec{ nullptr };
	Vec2* followedPos{ nullptr };

	Renderer renderer;

	float gravityAcceleration{ 0.0f };

	PhysicsMaterial material0;
	PhysicsMaterial material1;
};