#pragma once

#include <math/line.hpp>
#include <math/aabb.hpp>

#include <array>

struct LineSegment {
	LineSegment(Line line, float minDistanceAlongLine, float maxDistanceAlongLine);
	LineSegment(Vec2 start, Vec2 end);

	auto asBoxContains(float width, Vec2 p) const -> bool;
	auto asCapsuleContains(float thickness, Vec2 p) const -> bool;
	auto aabb() const -> Aabb;
	auto getCorners() const -> std::array<Vec2, 2>;
	// This is essentially just LineSegment vs LineSegment intersection
	auto raycastHit(Vec2 rayBegin, Vec2 rayEnd) -> std::optional<Vec2>;

	Line line;
	float minDistanceAlongLine;
	float maxDistanceAlongLine;
};