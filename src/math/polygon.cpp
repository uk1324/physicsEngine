#include <math/polygon.hpp>

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
	//auto twiceTheSignedArea = 0.0f;
	//auto f = [](Vec2 a, Vec2 b) {
	//	return (b.x - a.x) * (b.y + a.y);
	//};
	//twiceTheSignedArea += f(verts[0], verts[1]);
	//for (usize i = 1; i < verts.size() - 1; i++) {
	//	twiceTheSignedArea += f(verts[i], verts[i + 1]);
	//}
	//twiceTheSignedArea += f(verts[verts.size() - 1], verts[0]);

	return twiceTheSignedArea / 2.0f;
}

auto simplePolygonArea(Span<const Vec2> verts) -> float {
	return abs(simplePolygonArea(verts));
}

auto simplePolygonIsClockwise(Span<const Vec2> verts) -> bool {
	// If the signed area is negative then the vertices are counter-clockwise.
	return simplePolygonSignedArea(verts) > 0.0f;
}
