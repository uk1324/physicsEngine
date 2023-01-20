#pragma once

#include <gfx/gfx.hpp>
#include <game/renderer.hpp>

struct WaterDemo {
	WaterDemo();
	auto update(Gfx& gfx, Renderer& renderer) -> void;

	struct Cell {
		Vec2 direction{ 0.0f };
		float bottomY = 0.0f;
		i32 density = 0;
		bool isWall = false;
	};
	static constexpr i32 MAX_DENSITY = 8;

	usize frameNumber = 0;

	static constexpr Vec2T<i64> GRID_SIZE{ 40, 40 };
	Cell cells[GRID_SIZE.x][GRID_SIZE.y];

	static constexpr auto CELL_SIZE = 0.05f;
	static constexpr auto CELL_AREA = CELL_SIZE * CELL_SIZE;

	bool paused = false;
	bool singleStep = false;
};