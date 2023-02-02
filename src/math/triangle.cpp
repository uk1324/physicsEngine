#include <math/triangle.hpp>
#include <math/utils.hpp>

Triangle::Triangle(Vec2 v0, Vec2 v1, Vec2 v2)
	: v{ v0, v1, v2 } {}

auto Triangle::contains(Vec2 p) const -> bool {
	auto area0 = det(v[1] - v[0], p - v[0]);
	auto area1 = det(v[2] - v[1], p - v[1]);
	auto area2 = det(v[0] - v[2], p - v[2]);
	return sign(area0) == sign(area1) && sign(area1) == sign(area2);
	/*const auto EPSILON = 0.01f;
	return (area0 < EPSILON&& area1 < EPSILON && area2 < EPSILON)
		|| (area0 > -EPSILON && area1 > -EPSILON && area2 > -EPSILON);*/
}

auto Triangle::isClockwise() const -> bool {
	return det(v[0] - v[1], v[0] - v[2]) < 0.0f;
}

auto Triangle::area() -> bool {
	return abs(det(v[0] - v[1], v[0] - v[2])) / 2.0f;
}
