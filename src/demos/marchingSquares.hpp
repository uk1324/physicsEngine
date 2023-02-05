#pragma once

#include <engine/renderer.hpp>
#include <math/triangle.hpp>
#include <math/triangulate.hpp>

#include <vector>

struct MarchingSquares {
	MarchingSquares();
	auto update() -> void;


	auto marchingSquares() -> void;
	int i = 408;
	bool paused = false;
	std::vector<bool> visited;
	SimplePolygonTriangulator triangulate;
	DynamicTexture texture;
	std::vector<Vec2> starts;
	std::vector<std::vector<Vec2>> vertices;
};