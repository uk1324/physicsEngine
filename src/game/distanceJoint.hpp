#pragma once

#include <game/body.hpp>

struct DistanceJoint {
	BodyId bodyA, bodyB;
	float requiredDistance;
	// In the body's object space.
	Vec2 anchorOnA{ 0.0f }, anchorOnB{ 0.0f };

	float bias;

	auto preStep(float invDeltaTime) -> void;
	auto applyImpluse() -> void;

	auto getEndpoints() const -> std::array<Vec2, 2>;
};

using DistanceJointId = EntityArray<DistanceJoint>::Id;

