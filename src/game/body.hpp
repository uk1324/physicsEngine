#pragma once

#include <math/vec2.hpp>

struct Body {
	Body(Vec2 pos, Vec2 size, bool isStatic);

	auto isStatic() -> bool { return invMass == 0.0f; }

	Vec2 pos;
	Vec2 vel;
	Vec2 force;

	float orientation;
	float angularVel;
	float torque;

	Vec2 size;

	float coefficientOfFriction;
	float mass, invMass;
	float rotationalInertia, invRotationalInertia;
};