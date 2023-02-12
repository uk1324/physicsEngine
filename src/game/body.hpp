#pragma once

#include <math/transform.hpp>
#include <game/entityArray.hpp>
#include <game/collider.hpp>

struct Body {
	static constexpr float DEFAULT_DENSITY = 200.0f;

	Body();
	Body(Vec2 pos, const Collider& collider, bool isStatic = false);
	auto updateInvMassAndInertia() -> void;
	auto isStatic() const -> bool;
	auto makeStatic() -> void;
	auto updateMass(float density = DEFAULT_DENSITY) -> void;

	Transform transform;
	Vec2 vel;
	float angularVel;
	float mass;
	float rotationalInertia;
	float coefficientOfFriction;
	Collider collider;
	Vec2 force;
	float torque;
	float invMass;
	float invRotationalInertia;
};

using BodyId = EntityArray<Body>::Id;
