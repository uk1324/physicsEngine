#pragma once

#include <engine/renderer.hpp>
#include <math/triangle.hpp>
#include <math/triangulate.hpp>

#include <vector>

struct MarchingSquares {
	MarchingSquares();
	auto update() -> void;


	auto marchingSquares() -> void;
	/*int i = 1616;*/
	//int i = 3253;
	//int i = 3281;
	int i = 3286;
	bool asd = false;
	bool paused = false;
	std::vector<bool> visited;
	SimplePolygonTriangulator triangulate;
	DynamicTexture texture;
	std::vector<Vec2> starts;
	std::vector<std::vector<Vec2>> vertices;
};