#pragma once

#include <math/vec2.hpp>

struct Line {
	Line(Vec2 n, float d);

	// Unit length.
	Vec2 n;
	float d;
};

auto signedDistance(const Line& l, Vec2 p) -> float;
auto distance(const Line& l, Vec2 p) -> float;