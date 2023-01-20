#pragma once

#include <gfx/gfx.hpp>
#include <game/renderer.hpp>
#include <math/triangulate.hpp>


struct TriangulationDemo {
	TriangulationDemo();

	auto update(Gfx& gfx, Renderer& renderer) -> void;

	bool showTriangulation = true;
	int i = 0;
	int j = 0;

	std::vector<Triangle> triangles;
	std::vector<Vec2> points;
	SimplePolygonTriangulator triangulator;
};