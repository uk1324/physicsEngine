#pragma once

#include <game/body.hpp>

struct RevoluteJoint {
	BodyId bodyA, bodyB;
	// In the body's object space.
	Vec2 localAnchorA, localAnchorB;

	float bias;

	auto preStep(float invDeltaTime) -> void;
	auto applyImpluse() -> void;

	auto anchorsWorldSpace() const -> std::array<Vec2, 2>;

	Mat2 m;
	float motorSpeedInRadiansPerSecond = 0.0f;
};

using RevoluteJointId = EntityArray<RevoluteJoint>::Id;

