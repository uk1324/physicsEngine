#pragma once

#include <math/vec2.hpp>
#include <game/collision/collision.hpp>

struct Body {
	Body(Vec2 pos, const Collider& collider, bool isStatic);

	auto isStatic() const -> bool { return invMass == 0.0f; }

	Vec2 pos;
	Vec2 vel;
	Vec2 force;

	float orientation;
	float angularVel;
	float torque;

	Collider collider;

	float coefficientOfFriction;
	float mass, invMass;
	float rotationalInertia, invRotationalInertia;
};