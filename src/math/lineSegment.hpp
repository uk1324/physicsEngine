#pragma once

#include <math/line.hpp>

struct LineSegment {
	LineSegment(Vec2 start, Vec2 end);

	auto asBoxContains(float width, Vec2 p) -> bool;

	Line line;
	float minDistanceAlongLine;
	float maxDistanceAlongLine;
};