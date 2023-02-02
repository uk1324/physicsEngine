#pragma once

#include <engine/renderer.hpp>
#include <math/triangle.hpp>
#include <math/triangulate.hpp>

#include <vector>

struct MarchingSquares {
	MarchingSquares();
	auto update() -> void;


	auto marchingSquares() -> void;

	int i = 0;
	bool paused = true;
	SimplePolygonTriangulator triangulate;
	DynamicTexture texture;
	std::vector<std::vector<Vec2>> vertices;
	std::vector<std::pair<Vec2, Vec2>> lines;
};