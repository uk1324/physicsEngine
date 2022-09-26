#include <math/line.hpp>
#include <cmath>

Line::Line(Vec2 n, float d)
	: n{ n }
	, d{ d } {}

auto signedDistance(const Line& l, Vec2 p) -> float {
	return l.d - dot(l.n, p);
}

auto distance(const Line& l, Vec2 p) -> float {
	return abs(signedDistance(l, p));
}
