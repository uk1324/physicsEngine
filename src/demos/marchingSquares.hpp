#pragma once

#include <engine/renderer.hpp>
#include <math/triangle.hpp>

#include <vector>

struct MarchingSquares {
	MarchingSquares();
	auto update() -> void;

	DynamicTexture texture;
	std::vector<Vec2> vertices;
	std::vector<i32> values;
};