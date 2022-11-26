#pragma once

#include <game/renderer.hpp>
#include <game/bvhCollisionSystem.hpp>

// TODO: Replay system. Mouse position would need to be either saved in world space or later transformed by the camera transform. Could store 2 camera transforms one for the actual camera and the replay camera.

// Entites in the editor have to be polymorphic. Can't use inheritance because the pointers might get invalidated
/*
struct Entity {
	EntityType type;
	// Don't need to use verioning in the editor.
	i32 handle;
}

Accessing for example an entity's pos could be done polymorphically at runtime either using arrays which would require all of the arrays to be of the same type or using a switch (could use higher order macros).

The order in which the entites are added in the editor affects the order in which the collision constraints will be resolved at runtime. Can't think of any consistent way to deal with this.

Relational database theory. Seperate tables with 2 foreign keys.

https://box2d.org/documentation/md__d_1__git_hub_box2d_docs_loose_ends.html
*/

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