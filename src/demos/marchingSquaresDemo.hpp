#pragma once

#include <engine/renderer.hpp>
#include <math/triangle.hpp>
#include <math/triangulate.hpp>

#include <vector>

struct MarchingSquaresDemo {
	MarchingSquaresDemo();
	auto update() -> void;

	//int i = 1642;
	Camera camera;
	int frame = 409;
	bool drawImage = false;
	bool paused = true;
	bool pixelPerfect = false;
	bool connectDiagonals = false;
	std::vector<bool> visited;
	SimplePolygonTriangulator triangulate;
	DynamicTexture texture;
	std::vector<Vec2> starts;
	std::vector<std::vector<Vec2>> vertices;
};

