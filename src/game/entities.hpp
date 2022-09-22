#pragma once

#include <math/vec2.hpp>

#include <vector>

struct CircleEntity {
	Vec2 pos;
	Vec2 vel;
	float rotation;
	float rotationVel;

	float mass;

	float radius;
};

extern std::vector<CircleEntity> circleEntites;