#pragma once

#include <math/transform.hpp>
#include <game/entityArray.hpp>
#include <game/collider.hpp>

struct Body {
	Body();
	Body(Vec2 pos, const Collider& collider, bool isStatic);
	auto updateInvMassAndInertia() -> void;
	auto isStatic() const -> bool;

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
