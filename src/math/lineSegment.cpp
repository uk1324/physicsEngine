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

auto LineSegment::asBoxContains(float width, Vec2 p) -> bool {
	const auto along = line.distanceAlong(p);
	if (along < minDistanceAlongLine || along > maxDistanceAlongLine)
		return false;

	return distance(line, p) < width;
}
