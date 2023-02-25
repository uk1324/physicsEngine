#pragma once

#include <game/body.hpp>

// Do point to point constraints behave like 2 springs instead of one? Only the positional correction makes it a spring.
// The point to point constraint tries to prevent all the relative motion while to distance joint only prevent motion from the drift from body a to body b so it drifts even more from the 0 distance.
struct RevoluteJoint {
	BodyId bodyA, bodyB;
	// In the body's object space.
	Vec2 localAnchorA, localAnchorB;

	float bias;

	auto preStep(float invDeltaTime) -> void;
	auto applyImpluse() -> void;

	auto anchorsWorldSpace() const -> std::array<Vec2, 2>;

	Mat2 m;
};

using RevoluteJointId = EntityArray<RevoluteJoint>::Id;

