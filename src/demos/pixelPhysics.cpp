#include <demos/pixelPhysics.hpp>
#include <engine/window.hpp>
#include <engine/input.hpp>
#include <game/debug.hpp>

#include <imgui/imgui.h>
using namespace ImGui;

#include <random>

PixelPhysics::PixelPhysics(Gfx& gfx)
	: texture{ gfx, GRID_SIZE } {
	for (i64 x = 0; x < GRID_SIZE.x; x++) {
		for (i64 y = 0; y < GRID_SIZE.y; y++) {
			if (x == 0 || x == GRID_SIZE.x - 1 || y == 0 || y == GRID_SIZE.y - 1) {
				blocks[x][y] = Block::WALL;
			} else {
				blocks[x][y] = Block::AIR;
			}
		}
	}

}

auto PixelPhysics::update(Gfx& gfx, Renderer& renderer) -> void {
	const auto gridSize = CELL_SIZE * Vec2{ GRID_SIZE };
	// Set corner bottom left corner at [0, 0].
	const auto gridCenter = gridSize / 2.0f;
	camera.pos = gridCenter;
	// TODO: Maybe put into a function. 
	camera.setWidth(gridSize.x);
	if (gridSize.y > camera.height()) {
		camera.setWidth(gridSize.y * camera.aspectRatio);
	}

	Begin("pixels");
	Checkbox("paused", &paused);
	doSingleStep = false;
	if (paused && Button("single step")) {
		doSingleStep = true;
	}
	if (Button("reset")) {
		for (i64 x = 1; x < GRID_SIZE.x - 1; x++) {
			for (i64 y = 1; y < GRID_SIZE.y - 1; y++) {
				blocks[x][y] = Block::AIR;
			}
		}
	}
	// TODO: Add this to the data language. Could use the BeginCombo api instead of this.
	const char* items[BLOCK_TYPE_COUNT];
	for (auto& name : items) {
		name = "fixme";
	}
#define SET(type, name) items[static_cast<int>(Block::type)] = name
	SET(AIR, "air");
	SET(SAND, "sand");
	SET(WALL, "wall");
	SET(WATER, "water");
#undef SET
	auto selected = static_cast<int>(selectedBlock);
	Combo("selected block", &selected, items, std::size(items));
	selectedBlock = static_cast<Block>(selected);

	const i64 minRadius = 1, maxRadius = 10;
	SliderScalar("radius", ImGuiDataType_S64, &radius, &minRadius, &maxRadius);
	End();

	static std::uniform_int_distribution<int> rand(0, 1);
	static std::default_random_engine eng;
	if (!paused || doSingleStep) {
		frameNumber += 1;
		for (i64 y = 1; y < GRID_SIZE.y - 1; y++) {
			// !!!!The water is broken because the cells are always evaluated from left to right.
			for (i64 x = 1; x < GRID_SIZE.x - 1; x++) {

				if (frameNumber % 2 == 0) {
					bool updated = false;

					auto& block = blocks[x][y];
					auto& left = blocks[x - 1][y];
					auto& right = blocks[x + 1][y];
					auto& below = blocks[x][y - 1];
					auto& belowLeft = blocks[x - 1][y - 1];
					auto& belowRight = blocks[x + 1][y - 1];
					if (block == Block::SAND || block == Block::WATER) {
						if (below == Block::AIR) {
							below = block;
							block = Block::AIR;
							updated = true;
						} else {
							auto checkLeft = [&]() -> bool {
								if (left == Block::AIR && belowLeft == Block::AIR) {
									belowLeft = block;
									block = Block::AIR;
									updated = true;
									return true;
								}
								return false;
							};
							auto checkRight = [&]() -> bool {
								if (right == Block::AIR && belowRight == Block::AIR) {
									belowRight = block;
									block = Block::AIR;
									updated = true;
									return true;
								}
								return false;
							};

							/*if ((std::hash<i64>()(x) ^ std::hash<i64>()(y)) % 2 == 0) {*/
							if (rand(eng) % 2 == 0) {
								if (!checkLeft()) checkRight();
							} else {
								if (!checkRight()) checkLeft();
							}
						}
					}

					if (!updated && block == Block::WATER) {
						static int a, b;
						auto checkLeft = [&]() -> bool {
							if (left == Block::AIR) {
								a++;
								left = block;
								block = Block::AIR;
								updated = true;
								return true;
							}
							return false;
						};
						auto checkRight = [&]() -> bool {
							if (right == Block::AIR) {
								b++;
								right = block;
								block = Block::AIR;
								updated = true;
								return true;
							}
							return false;
						};

						if (rand(eng) % 2 == 0) {
							if (!checkLeft()) checkRight();
						} else {
							if (!checkRight()) checkLeft();
						}
					}
				}
			}
		}
	}

	const auto cursorPos = Vec2T<i64>{ (camera.cursorPos() / CELL_SIZE).applied(floor) };
	if (Input::isMouseButtonHeld(MouseButton::LEFT)) {
		const auto minPos = Vec2T<i64>{ 1 }, maxPos = GRID_SIZE - Vec2T<i64>{ 1 };
		const auto min = (cursorPos - Vec2T{ radius }).clamped(minPos, maxPos), max = (cursorPos + Vec2T{ radius }).clamped(minPos, maxPos);
		for (i64 x = min.x; x < max.x; x++) {
			for (i64 y = min.y; y < max.y; y++) {
				auto& block = blocks[x][y];
				if (block != Block::AIR && selectedBlock != Block::AIR)
					continue;

				if (distance(Vec2{ Vec2T{ x, y } } + Vec2{ 0.5f }, Vec2{ cursorPos } + Vec2{ 0.5f }) < radius - 0.5f) {
					block = selectedBlock;	
				}
			}
		}
	}

	if (Input::scrollDelta() > 0.0f) {
		radius = std::min(radius + 1, maxRadius);
	} else if (Input::scrollDelta() < 0.0f) {
		radius = std::max(radius - 1, minRadius);
	}

	for (auto& p : texture.indexed()) {
		auto gridPos = p.pos;
		gridPos.y = GRID_SIZE.y - 1 - p.pos.y;
		const auto block = blockAt(gridPos);
		switch (block) {
		case Block::AIR: p = PixelRgba::BLACK; break;
		case Block::SAND: p = PixelRgba{ 255, 255, 0 }; break;
		case Block::WALL: p = PixelRgba{ 128 }; break;
		case Block::WATER: p = PixelRgba{ 0, 0, 255 }; break;
		}
	}

	renderer.drawDynamicTexture(gridCenter, gridSize.y, texture);
	renderer.update(gfx, camera);
}

auto PixelPhysics::blockAt(Vec2T<i64> pos) -> Block& {
	return blocks[pos.x][pos.y];
}