#pragma once

#include <math/triangulate.hpp>


struct TriangulationDemo {
	auto update() -> void;

	bool showTriangulation = true;
	int i = 0;
	int j = 0;

	std::vector<Triangle> triangles;
	std::vector<Vec2> points;
	SimplePolygonTriangulator triangulator;
};