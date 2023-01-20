#pragma once

#include <math/triangle.hpp>
#include <utils/span.hpp>

#include <vector>

struct SimplePolygonTriangulator {
	// The resulting triangles are clockwise.
	auto operator()(Span<const Vec2> vertices) -> const std::vector<Triangle>&;

private:
	std::vector<Vec2> verts;
	std::vector<Triangle> result;
};