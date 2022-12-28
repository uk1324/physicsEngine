#include <math/lineSegment.hpp>

LineSegment::LineSegment(Vec2 start, Vec2 end) 
	: line{ start, end } {
	float a = line.distanceAlong(start);
	float b = line.distanceAlong(end);
	if (a < b) {
		minDistanceAlongLine = a;
		maxDistanceAlongLine = b;
	} else {
		minDistanceAlongLine = b;
		maxDistanceAlongLine = a;
	}
}

auto LineSegment::asBoxContains(float halfWidth, Vec2 p) const -> bool {
	const auto along = line.distanceAlong(p);
	if (along < minDistanceAlongLine || along > maxDistanceAlongLine)
		return false;

	return distance(line, p) < halfWidth;
}

auto LineSegment::asCapsuleContains(float thickness, Vec2 p) const -> bool {
	const auto along = line.distanceAlong(p);
	if (along < minDistanceAlongLine || along > maxDistanceAlongLine) {
		const auto alongClamped = std::clamp(along, minDistanceAlongLine, maxDistanceAlongLine);
		const auto lineEdge = line.n.rotBy90deg() * alongClamped;
		return distance(lineEdge, p) < thickness;
	}
	return distance(line, p) < thickness;
}

auto LineSegment::aabb() const -> Aabb {
	const auto alongLine = -line.n.rotBy90deg();
	const auto offset = line.n * line.d;
	return Aabb::fromCorners(alongLine * minDistanceAlongLine + offset, alongLine * maxDistanceAlongLine + offset);
}
