#pragma once

#include <game/body.hpp>

struct DistanceJoint {
	BodyId bodyA, bodyB;
	float requiredDistance;
	Vec2 posOnA, posOnB;

	float bias;

	auto preStep(float invDeltaTime) -> void;
	auto applyImpluse() -> void;
};

using DistanceJointId = EntityArray<DistanceJoint>::Id;

