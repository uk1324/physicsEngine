#pragma once

#include <gfx/gfx.hpp>
#include <game/renderer.hpp>

struct Triangle {
	Vec2 verts[3];
};

struct TriangulationDemo {
	TriangulationDemo();

	auto update(Gfx& gfx, Renderer& renderer) -> void;

	std::vector<Vec2> points;

	bool showTriangulation = true;
	int i = 0;

	int j = 0;

	std::vector<Triangle> triangles;
};