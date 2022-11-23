#pragma once

#include <math/vec2.hpp>

struct Body;

struct DistanceJoint {
	Vec2 posOnA, posOnB;
	float requiredDistance;

	float bias;

	auto preStep(Body& a, Body& b, float invDeltaTime) -> void;
	auto applyImpluse(Body& a, Body& b) -> void;
};