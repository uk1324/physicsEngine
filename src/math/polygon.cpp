#include <math/polygon.hpp>
#include <math/line.hpp>

template<typename T>
static auto getWithWrapAround(const T& array, i64 index) -> auto {
	// TODO: Wrap around points like -2 or size + 2 correctly.
	if (index < 0) {
		index = array.size() - 1;
	} else if (index >= array.size()) {
		index = 0;
	}
	return array[index];
}

auto simplePolygonSignedArea(Span<const Vec2> verts) -> float {
	// Formula for signed area explained here https://gamemath.com/book/geomprims.html - 9.6.2 Area of a Triangle
	// This is called the shoelace formula. This formula and other formulas for calculating the area of a simple polygon are described here. 
	// https://en.wikipedia.org/wiki/Shoelace_formula
	// Determinant formula
	// https://mathworld.wolfram.com/PolygonArea.html
	if (verts.size() < 3) {
		return 0.0f;
	}
	auto twiceTheSignedArea = 0.0f;
	auto previous = verts.size() - 1 ;
	for (usize i = 1; i < verts.size(); i++) {
		twiceTheSignedArea += (verts[i].x - verts[previous].x) * (verts[i].y + verts[previous].y);
		previous = i;
	}

	return twiceTheSignedArea / 2.0f;
}

auto simplePolygonArea(Span<const Vec2> verts) -> float {
	return abs(simplePolygonArea(verts));
}

auto simplePolygonIsClockwise(Span<const Vec2> verts) -> bool {
	// If the signed area is negative then the vertices are counter-clockwise.
	return simplePolygonSignedArea(verts) > 0.0f;
}

auto lineSegmentIntersectsSimplePolygon(Vec2 start, Vec2 end, Span<const Vec2> verts) -> bool {
	return false;
}

auto polygonDouglassPeckerSimplify(Span<const Vec2> verts, float maxDistanceFromLine) -> std::vector<Vec2> {
	if (verts.size() <= 2) {
		return std::vector<Vec2>{ verts.begin(), verts.end() };
	}

	auto first = verts[0];
	// why getWithWrapAround?
	auto last = getWithWrapAround(verts, verts.size() - 1);

	auto maxDistance = -std::numeric_limits<float>::infinity();
	i64 maxVertex = 1;
	for (i64 i = 1; i < verts.size() - 1; i++) {
		const auto d = distance(Line{ first, last }, verts[i]);
		if (d > maxDistance) {
			maxDistance = d;
			maxVertex = i;
		}
	}

	if (maxDistance >= maxDistanceFromLine) {
		std::vector<Vec2> one = polygonDouglassPeckerSimplify(std::vector<Vec2>(verts.begin() + 0, verts.begin() + maxVertex + 1), maxDistanceFromLine);
		std::vector<Vec2> two = polygonDouglassPeckerSimplify(std::vector<Vec2>(verts.begin() + maxVertex, verts.begin() + verts.size()), maxDistanceFromLine);
		// rejoin the two (TODO without repeating the middle point)
		std::vector<Vec2> simplified;
		simplified.insert(simplified.end(), one.begin(), one.end());
		simplified.insert(simplified.end(), two.begin(), two.end());
		return simplified;
	} else {
		std::vector<Vec2> simplified{ first, last };
		return simplified;
	}
}