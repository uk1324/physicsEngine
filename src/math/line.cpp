#include <math/line.hpp>
#include <cmath>

Line::Line(Vec2 n, float d)
	: n{ n }
	, d{ d } {}

auto Line::translated(Vec2 v) const -> Line {
	return Line{ n, d - dot(n, v) };
}

auto signedDistance(const Line& l, Vec2 p) -> float {
	return l.d + dot(l.n, p);
}

auto distance(const Line& l, Vec2 p) -> float {
	return abs(signedDistance(l, p));
}
