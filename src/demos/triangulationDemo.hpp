#pragma once

#include <gfx/gfx.hpp>
#include <game/renderer.hpp>

struct TriangulationDemo {
	TriangulationDemo();

	auto update(Gfx& gfx, Renderer& renderer) -> void;

	std::vector<Vec2> points;
};