#pragma once

#include <math/vec2.hpp>
struct Line {
	Line(Vec2 n, float d);
	// The normal is the line from a to b rotated counterclockwise.
	Line(Vec2 a, Vec2 b);
	auto translated(Vec2 v) const -> Line;

	// dot(n, p) = d
	// This means that the line center is located at the vector n * d and the positive half space lies in the direction of the normal.

	// Unit length.
	Vec2 n;
	float d;
};

auto signedDistance(const Line& l, Vec2 p) -> float;
auto distance(const Line& l, Vec2 p) -> float;