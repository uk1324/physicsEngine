#pragma once

#include <gfx/gfx.hpp>
#include <game/renderer.hpp>

// Compute the vertical pressure.
// Iterate from top to bottom. if water then Pressure = above pressure + 1 else 0
// Could base the whole simulation on pressure. The issue is that compute both horizontal and vertial pressure you would need to solve s system of equations. I think, not sure.
struct WaterDemo {
	WaterDemo();
	auto update(Gfx& gfx, Renderer& renderer) -> void;
	auto reset() -> void;

	struct Cell {
		i32 density = 0;
		bool isWall = false;
	};
	static constexpr i32 MAX_DENSITY = 16;

	usize frameNumber = 0;

	static constexpr Vec2T<i64> GRID_SIZE{ 15, 15 };
	Cell cells[GRID_SIZE.x][GRID_SIZE.y];

	Vec2T<i64> selected{ 0, 0 };

	static constexpr auto CELL_SIZE = 0.05f;
	static constexpr auto CELL_AREA = CELL_SIZE * CELL_SIZE;

	bool paused = false;
	bool singleStep = false;
	bool drawGrid = false;
	bool showDensity = false;
};