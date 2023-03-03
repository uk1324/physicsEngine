#pragma once

#include <game/body.hpp>

struct SpringJoint {
	BodyId bodyA, bodyB;
	float restLength = 1.0f;
	Vec2 localAnchorA{ 0.0f }, localAnchorB{ 0.0f };

	auto preStep(float invDeltaTime) -> void;
	auto applyImpluse() -> void;

	float invDt;

	Mat2 m;
};

using SpringJointId = EntityArray<SpringJoint>::Id;
