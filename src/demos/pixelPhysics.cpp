#include <demos/pixelPhysics.hpp>
#include <engine/window.hpp>
#include <engine/input.hpp>
#include <engine/debug.hpp>
#include <utils/staticVec.hpp>
#include <math/utils.hpp>

#include <imgui/imgui.h>
using namespace ImGui;

#include <random>

PixelPhysics::PixelPhysics()
	: texture{ GRID_SIZE } {
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

auto PixelPhysics::update() -> void {
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
	SET(PLANT, "plant");
	SET(FLOWER, "flower");
	SET(FIRE, "fire");
	SET(WOOD, "wood");
	SET(TERMITE, "termite");
#undef SET
	auto selected = static_cast<int>(selectedBlock);
	Combo("selected block", &selected, items, static_cast<int>(std::size(items)));
	selectedBlock = static_cast<Block>(selected);

	const i64 minRadius = 1, maxRadius = 10;
	SliderScalar("radius", ImGuiDataType_S64, &radius, &minRadius, &maxRadius);

	SliderInt("fire spread chance", &fireSpreadChance, 1, 20);
	End();

	static std::uniform_int_distribution<int> rand(0, 1);
	static std::default_random_engine eng;

	auto update = [this](i64 x, i64 y) -> void {
		auto updated = false;
		if (frameNumber % 2 == 0) {

			auto& block = blocks[x][y];
			auto& left = blocks[x - 1][y];
			auto& right = blocks[x + 1][y];
			auto& below = blocks[x][y - 1];
			auto& belowLeft = blocks[x - 1][y - 1];
			auto& belowRight = blocks[x + 1][y - 1];
			if (block == Block::SAND || block == Block::WATER) {
				auto doSwap = [&](Block type) -> bool {
					if (block == Block::SAND) {
						return type == Block::AIR || type == Block::WATER;
					} else if (block == Block::WATER) {
						return type == Block::AIR;
					}
					return false;
				};

				if (below == Block::AIR) {
					std::swap(block, below);
					updated = true;
				} else {
					auto checkLeft = [&]() -> bool {
						if (doSwap(left) && doSwap(belowLeft)) {
							std::swap(block, belowLeft);
							updated = true;
							return true;
						}
						return false;
					};
					auto checkRight = [&]() -> bool {
						if (doSwap(right) && doSwap(belowRight)) {
							std::swap(block, belowRight);
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

			/*if (!updated && block == Block::SAND && below == Block::WATER) {
				std::swap(block, below);
				updated = true;
			}*/

			if (!updated && block == Block::WATER) {
				auto checkLeft = [&]() -> bool {
					if (left == Block::AIR) {
						std::swap(block, left);
						updated = true;
						return true;
					}
					return false;
				};
				auto checkRight = [&]() -> bool {
					if (right == Block::AIR) {
						std::swap(block, right);
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
	};

	if (!paused || doSingleStep) {
		frameNumber += 1;
		for (i64 y = 1; y < GRID_SIZE.y - 1; y++) {
			if (rand(eng) % 2 == 0) {
				for (i64 x = 1; x < GRID_SIZE.x - 1; x++) update(x, y);
			} else {
				for (i64 x = GRID_SIZE.x - 2; x > 2; x--) update(x, y);
			}
		}

		auto updateWater = [&](i64 x, i64 y) {
			auto& block = blocks[x][y];
			auto& above = blocks[x][y + 1];
			auto& left = blocks[x - 1][y];
			auto& right = blocks[x + 1][y];
			if (block == Block::WATER && above == Block::SAND) {
				if (left == Block::AIR) {
					std::swap(block, left);
					std::swap(block, above);
				} else if (right == Block::AIR) {
					std::swap(block, right);
					std::swap(block, above);
				} else {
					std::swap(block, above);
				}
			}
		};

		// Iterating up and swapping block with up isn't the same as iterating down and swapping down. And the same is true with replaced down with up in the first and second part. In any order (up, up) (down, down) or (up, down), (down, up)
		if (frameNumber % 4 == 0) {
			// Using first loop y up second loop rand x also works well
			for (i64 x = 1; x < GRID_SIZE.x - 1; x++) {
				for (i64 y = 1; y < GRID_SIZE.y - 1; y++) updateWater(x, y);
			}
		}

		if (frameNumber % 8 == 0) {
			for (auto& col : grewThisFrame) {
				for (auto& v : col) {
					v = false;
				}
			}

			for (i64 x = 1; x < GRID_SIZE.x - 1; x++) {
				for (i64 y = 1; y < GRID_SIZE.y - 1; y++) {
					if (blocks[x][y] != Block::PLANT || grewThisFrame[x][y])
						continue;
					StaticVec<Vec2T<i64>, 4> directions;
					/*if (blocks[x + 1][y] == Block::AIR) directions.push({ x + 1, y });
					if (blocks[x - 1][y] == Block::AIR) directions.push({ x - 1, y });
					if (blocks[x][y + 1] == Block::AIR) directions.push({ x, y + 1 });
					if (blocks[x][y - 1] == Block::AIR) directions.push({ x, y - 1 });*/

					if (blocks[x + 1][y] == Block::AIR && blocks[x + 1][y - 1] != Block::PLANT && blocks[x + 1][y + 1] != Block::PLANT) directions.push({ x + 1, y });
					if (blocks[x - 1][y] == Block::AIR && blocks[x - 1][y - 1] != Block::PLANT && blocks[x - 1][y + 1] != Block::PLANT) directions.push({ x - 1, y });
					if (blocks[x][y + 1] == Block::AIR && blocks[x + 1][y + 1] != Block::PLANT && blocks[x - 1][y + 1] != Block::PLANT) directions.push({ x, y + 1 });
					if (blocks[x][y - 1] == Block::AIR && blocks[x + 1][y - 1] != Block::PLANT && blocks[x - 1][y - 1] != Block::PLANT) directions.push({ x, y - 1 });

					usize i = 0;
					if (blocks[x + 1][y] == Block::PLANT) i++;
					if (blocks[x - 1][y] == Block::PLANT) i++;
					if (blocks[x][y + 1] == Block::PLANT) i++;
					if (blocks[x][y - 1] == Block::PLANT) i++;

					if ((std::hash<i64>()(x) ^ std::hash<i64>()(y)) % 5 == 0) {
						if ((blocks[x + 1][y] == Block::PLANT && blocks[x - 1][y] == Block::PLANT && blocks[x][y + 1] == Block::AIR && blocks[x][y - 1] == Block::AIR)) {
							if (std::uniform_int_distribution<int>(0, 1)(eng) % 2 == 0) {
								blocks[x][y + 1] = Block::FLOWER;
							} else {
								blocks[x][y - 1] = Block::FLOWER;
							}
						}

						if ((blocks[x][y + 1] == Block::PLANT && blocks[x][y - 1] == Block::PLANT && blocks[x + 1][y] == Block::AIR && blocks[x - 1][y] == Block::AIR)) {
							if (std::uniform_int_distribution<int>(0, 1)(eng) % 2 == 0) {
								blocks[x + 1][y] = Block::FLOWER;
							} else {
								blocks[x - 1][y] = Block::FLOWER;
							}
						}
					}

					if (i < 2 && directions.size() != 0) {
						const auto dir = directions[std::uniform_int_distribution<usize>(0, directions.size() - 1)(eng)];
						blocks[dir.x][dir.y] = Block::PLANT;
						grewThisFrame[dir.x][dir.y] = true;
					}
					
				}
			}
		}

		if (frameNumber % 2 == 0) {
			for (i64 x = 1; x < GRID_SIZE.x - 1; x++) {
				auto canBurn = [](Block type) {
					switch (type) {
					case PixelPhysics::Block::PLANT: 
					case PixelPhysics::Block::FLOWER:
					case PixelPhysics::Block::WOOD:
						return true;
					default:
						return false;
					}
				};

				/*for (i64 y = 1; y < GRID_SIZE.y - 1; y++) {*/
				for (i64 y = GRID_SIZE.y - 2; y > 0; y--) {
					if (blocks[x][y] != Block::FIRE)
						continue;

					auto& fuelLeft = fireFuelLeft[x][y];
					if (fuelLeft == 0) {
						blocks[x][y] = Block::AIR;
						continue;
					}
					fuelLeft--;

					StaticVec<Vec2T<i64>, 4> directions;
					if (canBurn(blocks[x + 1][y])) directions.push({ x + 1, y });
					if (canBurn(blocks[x - 1][y])) directions.push({ x - 1, y });
					if (canBurn(blocks[x][y + 1])) directions.push({ x, y + 1 });
					if (canBurn(blocks[x][y - 1])) directions.push({ x, y - 1 });

					if (::rand() % fireSpreadChance == 0 && directions.size() != 0) {
						const auto dir = directions[std::uniform_int_distribution<usize>(0, directions.size() - 1)(eng)];
						blocks[dir.x][dir.y] = Block::FIRE;
						fireFuelLeft[dir.x][dir.y] = FIRE_MAX;
					} else {
						StaticVec<Vec2T<i64>, 7> airDirections;
						if (blocks[x + 1][y] == Block::AIR) airDirections.push({ x + 1, y });
						if (blocks[x - 1][y] == Block::AIR) airDirections.push({ x - 1, y });
						if (blocks[x][y + 1] == Block::AIR) {
							airDirections.push({ x, y + 1 });
							airDirections.push({ x, y + 1 });
						}
						if (blocks[x][y - 1] == Block::AIR) airDirections.push({ x, y - 1 });

						if (airDirections.size() != 0) {
							const auto dir = airDirections[std::uniform_int_distribution<usize>(0, airDirections.size() - 1)(eng)];
							std::swap(blocks[x][y], blocks[dir.x][dir.y]);
							fireFuelLeft[dir.x][dir.y] = fireFuelLeft[x][y];
						}
					}

					
				}
			}
		}

		for (auto& col : termiteUpdatedThisFrame) {
			for (auto& v : col) {
				v = false;
			}
		}

		for (i64 x = 1; x < GRID_SIZE.x - 1; x++) {
			for (i64 y = 1; y < GRID_SIZE.y - 1; y++) {
				if (blocks[x][y] != Block::TERMITE || termiteUpdatedThisFrame[x][y])
					continue;

				StaticVec<Vec2T<i64>, 4> nonAirDirections;
				if (blocks[x + 1][y] != Block::WALL && blocks[x + 1][y] != Block::AIR) nonAirDirections.push({ x + 1, y });
				if (blocks[x - 1][y] != Block::WALL && blocks[x - 1][y] != Block::AIR) nonAirDirections.push({ x - 1, y });
				if (blocks[x][y + 1] != Block::WALL && blocks[x][y + 1] != Block::AIR) nonAirDirections.push({ x, y + 1 });
				if (blocks[x][y - 1] != Block::WALL && blocks[x][y - 1] != Block::AIR) nonAirDirections.push({ x, y - 1 });

				StaticVec<Vec2T<i64>, 4> airDirections;
				if (blocks[x + 1][y] == Block::AIR) airDirections.push({ x + 1, y });
				if (blocks[x - 1][y] == Block::AIR) airDirections.push({ x - 1, y });
				if (blocks[x][y + 1] == Block::AIR) airDirections.push({ x, y + 1 });
				if (blocks[x][y - 1] == Block::AIR) airDirections.push({ x, y - 1 });

				Vec2T<i64> moveDir;
				if (nonAirDirections.size() == 0 && airDirections.size() != 0) {
					moveDir = airDirections[std::uniform_int_distribution<usize>(0, airDirections.size() - 1)(eng)];
				} else if (nonAirDirections.size() != 0) {
					moveDir = nonAirDirections[std::uniform_int_distribution<usize>(0, nonAirDirections.size() - 1)(eng)];
				} else {
					continue;
				}

				blocks[x][y] = Block::AIR;
				blocks[moveDir.x][moveDir.y] = Block::TERMITE;
				termiteUpdatedThisFrame[moveDir.x][moveDir.y] = true;
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
					if (selectedBlock == Block::FIRE) {
						fireFuelLeft[x][y] = FIRE_MAX;
					}
				}
			}
		}
	}

	if (!Input::isMouseButtonHeld(MouseButton::LEFT) || selectedBlock == Block::AIR) {
		Debug::drawHollowCircle(camera.cursorPos(), static_cast<float>(radius));
	}

	if (Input::scrollDelta() > 0.0f) {
		radius = std::min(radius + 1, maxRadius);
	} else if (Input::scrollDelta() < 0.0f) {
		radius = std::max(radius - 1, minRadius);
	}

	auto hashPos = [](i64 x, i64 y) -> usize {
		//return std::hash<usize>()(std::hash<i64>()(x) ^ std::hash<i64>()(y));
		return (std::hash<i64>()(x)) * 17 ^ std::hash<i64>()(y) * 587;
	};

	srand(static_cast<int>(frameNumber / 2));

	for (auto& p : texture.indexed()) {
		auto gridPos = p.pos;
		gridPos.y = GRID_SIZE.y - 1 - p.pos.y;
		const auto block = blockAt(gridPos);
		switch (block) {
		case Block::AIR: p = PixelRgba::BLACK; break;
		case Block::SAND: {
			const PixelRgba colors[] = { { 239 - 5, 221 - 10, 111  }, { 239 - 10, 221 - 20, 111  }, { 239 - 15, 221 - 30, 111  }, { 239 - 20, 221 - 40, 111  } };
			p = colors[hashPos(gridPos.x, gridPos.y) % 4];
			break;
		}
		case Block::WALL: p = PixelRgba{ 128 }; break;
		case Block::WATER: {
			/*const PixelRgba colors[] = { { 9,144,222 }, { 8,130,200 }, {9,99,151}, {9,78,118} };*/
			const PixelRgba colors[] = { { 9, 144 - 10, 222 }, { 9, 144 - 20, 222 + 2}, { 9, 144 - 30, 222 + 4 }, { 9, 144 - 40, 222 + 6 } };
			p = colors[::rand() % 4];
			break;
		}
		// To get more intersting colors choose the color based on the hash of the position. This won't probably won't look very well for slow moving things. For fast moving things it might look ok.
		case Block::PLANT: {
			auto h = hashPos(gridPos.x, gridPos.y);
			switch (h % 4) {
			case 0:  p = PixelRgba{ 69,242,72 }; break;
			case 1:  p = PixelRgba{ 45,184,61 }; break;
			case 2:  p = PixelRgba{ 133,255,122 }; break;
			case 3:  p = PixelRgba{ 76,208,56 }; break;
			}
			break;
		}
		case Block::FLOWER: p = PixelRgba{ 255, 0, 0 }; break;
		case Block::FIRE: {
			const auto c = lerp(Vec3{ 255, 232, 8 } / 255.0f, Vec3{ 255, 0, 0 } / 255.0f, fireFuelLeft[gridPos.x][gridPos.y] / float(FIRE_MAX));
			p = PixelRgba{ c };
			break;
		}
		case Block::WOOD: p = PixelRgba{ 215, 186, 137 }; break;
		case Block::TERMITE: p = PixelRgba{ 64 }; break;
		default: ASSERT_NOT_REACHED(); break;
		}
	}

	Renderer::drawDynamicTexture(gridCenter, gridSize.y, texture);
	Renderer::update(camera);
}

auto PixelPhysics::blockAt(Vec2T<i64> pos) -> Block& {
	return blocks[pos.x][pos.y];
}