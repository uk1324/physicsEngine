#pragma once

#include <math/vec2.hpp>

#include <vector>

struct PhysicsMaterial {
	float bounciness;
};

struct CircleEntity {
	Vec2 pos;
	Vec2 vel;
	float rotation;
	float angularVel;

	float mass;

	float radius;

	const PhysicsMaterial* material;
};

extern std::vector<CircleEntity> circleEntites;