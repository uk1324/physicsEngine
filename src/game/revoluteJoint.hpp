#pragma once

#include <game/body.hpp>

struct RevoluteJoint {
	BodyId bodyA, bodyB;
	// In the body's object space.
	Vec2 localAnchorA, localAnchorB;
	float motorSpeedInRadiansPerSecond = 0.0f;

	float bias;

	auto preStep(float invDeltaTime) -> void;
	auto applyImpluse() -> void;

	auto anchorsWorldSpace() const -> std::array<Vec2, 2>;

	Mat2 m;
};

using RevoluteJointId = EntityArray<RevoluteJoint>::Id;

