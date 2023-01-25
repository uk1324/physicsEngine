#pragma once

#include <gfx/gfx.hpp>
#include <game/renderer.hpp>

// Compute the vertical pressure.
// Iterate from top to bottom. if water then Pressure = above pressure + 1 else 0
// Could base the whole simulation on pressure. The issue is that compute both horizontal and vertial pressure you would need to solve s system of equations. I think, not sure.

// One thing that is probably not worth implementing is making the visuals actually preserve volume. This might even look weird in some cases. Like how would water falling down look if it doesn't take the full with of a block. It is easier to preserve the volume in the code logic and just make it look smooth in the drawing logic. Could even do something with signed distance fields to make the shapes really smooth. Not sure how to implement this. Just simply bilinearlly interpolating the values doesn't look good.

// Could try interpolating the water falling down in between frames. This might not work out thought, because it might break other water moving, although it shouldn't if it is strictly done in between frames. Not sure if it would look good or just weird.
struct WaterDemo {
	WaterDemo();
	auto update(Gfx& gfx, Renderer& renderer) -> void;
	auto reset() -> void;

	struct Cell {
		i32 density = 0;
		bool isWall = false;
	};
	// Modify this.
	static constexpr i32 MAX_DENSITY = 16;

	usize frameNumber = 0;

	static constexpr Vec2T<i64> GRID_SIZE{ 45, 45 };
	Cell cells[GRID_SIZE.x][GRID_SIZE.y];

	Vec2T<i64> selected{ 0, 0 };

	static constexpr auto CELL_SIZE = 0.05f;
	static constexpr auto CELL_AREA = CELL_SIZE * CELL_SIZE;

	bool paused = false;
	bool singleStep = false;
	bool drawGrid = false;
	bool showDensity = false;
};