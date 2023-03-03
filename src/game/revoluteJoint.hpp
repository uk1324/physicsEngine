#pragma once

#include <game/body.hpp>

struct RevoluteJoint {
	BodyId bodyA, bodyB;
	Vec2 localAnchorA{ 0.0f }, localAnchorB{ 0.0f };
	float motorSpeedInRadiansPerSecond = 0.0f;

	float bias;

	auto preStep(float invDeltaTime) -> void;
	auto applyImpluse() -> void;

	auto anchorsWorldSpace() const -> std::array<Vec2, 2>;

	Mat2 m;
};

using RevoluteJointId = EntityArray<RevoluteJoint>::Id;

