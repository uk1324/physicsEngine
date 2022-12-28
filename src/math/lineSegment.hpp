#pragma once

#include <math/line.hpp>
#include <math/aabb.hpp>

struct LineSegment {
	LineSegment(Vec2 start, Vec2 end);

	auto asBoxContains(float width, Vec2 p) const -> bool;
	auto asCapsuleContains(float thickness, Vec2 p) const -> bool;
	auto aabb() const -> Aabb;

	Line line;
	float minDistanceAlongLine;
	float maxDistanceAlongLine;
};