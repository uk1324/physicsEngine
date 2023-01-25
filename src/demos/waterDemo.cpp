#include <demos/waterDemo.hpp>
#include <game/debug.hpp>
#include <engine/time.hpp>
#include <game/input.hpp>

#include <imgui/imgui.h>
using namespace ImGui;

WaterDemo::WaterDemo() {
	reset();
}

auto WaterDemo::update(Gfx& gfx, Renderer& renderer) -> void {
	Camera camera;
	const auto gridSize = CELL_SIZE * Vec2{ GRID_SIZE };
	// Set corner bottom left corner at [0, 0].
	const auto gridCenter = gridSize / 2.0f;
	camera.pos = gridCenter;
	// TODO: Maybe put into a function. 
	camera.setWidth(gridSize.x);
	if (gridSize.y > camera.height()) {
		camera.setWidth(gridSize.y * camera.aspectRatio);
	}


	frameNumber++;
	if ((!paused && frameNumber % 4 == 0) || singleStep) {

		for (i64 y = 1; y < GRID_SIZE.y - 1; y++) {
			for (i64 x = 1; x < GRID_SIZE.x - 1; x++) {
				auto& cell = cells[x][y];
				if (cell.isWall)
					continue;

				auto& below = cells[x][y - 1];
				if (!below.isWall) {
					below.density += cell.density;
					const auto over = std::max(0, below.density - MAX_DENSITY);
					below.density -= over;
					cell.density = over;
				}
			}

			// Separating the left and right flowing into 2 loops to make it as order independed as possible. This still does different things for different directions, but it isn't very visible. And if it is, it doesn't look weird.
			for (i64 x = GRID_SIZE.x - 2; x > 0; x--) {
				auto& cell = cells[x][y];
				if (cell.isWall)
					continue;

				auto& right = cells[x + 1][y];
				if (!right.isWall && right.density < cell.density) {
					auto difference = cell.density - right.density;
					if (difference > 1) {
						difference /= 2;
					}
					cell.density -= difference;
					right.density += difference;
				}
			}

			for (i64 x = 1; x < GRID_SIZE.x - 1; x++) {
				auto& cell = cells[x][y];
				if (cell.isWall)
					continue;

				auto& left = cells[x - 1][y];
				if (!left.isWall && left.density < cell.density) {
					auto difference = cell.density - left.density;
					if (difference > 1) {
						difference /= 2;
					}
					cell.density -= difference;
					left.density += difference;
				}
			}
		}
	}

	for (i64 x = 0; x < GRID_SIZE.x; x++) {
		for (i64 y = 0; y < GRID_SIZE.y; y++) {
			const auto& cell = cells[x][y];
			if (cell.isWall) {
				const auto pos = Vec2{ Vec2T{ x, y } } *CELL_SIZE;
				const Vec2 verts[] = {
					pos,
					pos + Vec2{ 0, CELL_SIZE },
					pos + Vec2{ CELL_SIZE, CELL_SIZE },
					pos + Vec2{ CELL_SIZE, 0 }
				};
				Debug::drawSimplePolygon(verts, Vec3{ 0.5f });
			}
		}
	}

	for (i64 x = 1; x < GRID_SIZE.x - 1; x++) {
		for (i64 y = 1; y < GRID_SIZE.y - 1; y++) {
			const auto& cell = cells[x][y];
			ASSERT(cell.density <= MAX_DENSITY);
			
			if (cell.isWall) {
				
			} else if (cells[x][y].density > 0.0f) {
				const auto pos = Vec2{ Vec2T{ x, y } } * CELL_SIZE;

				if (drawGrid) {
					Vec2 v[] = {
						pos,
						pos + Vec2{ CELL_SIZE, 0 },
						pos + Vec2{ CELL_SIZE, CELL_SIZE },
						pos + Vec2{ 0, CELL_SIZE }
					};
					Debug::drawLines(v, Vec3::WHITE);
				}

				const auto height = cell.density;
				std::vector<Vec2> verts;
				verts.push_back(pos);
				verts.push_back(pos + Vec2{ CELL_SIZE, 0 });

				auto getHeight = [](const Cell& cell) -> float {
					return cell.density / static_cast<float>(MAX_DENSITY) * CELL_SIZE;
				};

				auto getH = [&](const Vec2T<i64>& cell) -> float {
					if (cells[cell.x][cell.y + 1].density > 0)
						return CELL_SIZE;
					return cells[cell.x][cell.y].density / static_cast<float>(MAX_DENSITY) * CELL_SIZE;
				};

				auto& right = cells[x + 1][y];
				auto& left = cells[x - 1][y];
				auto& belowRight = cells[x + 1][y - 1];
				auto& belowLeft = cells[x - 1][y - 1];
				if (cells[x][y + 1].density == 0) {

					// Techincally more correct, but without it the water looks smoother. Adds a vertex in the middle if both neighbours are either bigger or smaller.
										
					const auto flowingFromRight = !right.isWall && right.density > cell.density;
					const auto flowingFromLeft = !left.isWall && left.density > cell.density;
					if (flowingFromLeft && flowingFromRight) {
						verts.push_back(pos + Vec2{ CELL_SIZE, getH(Vec2T{ x + 1, y }) });
						verts.push_back(pos + Vec2{ CELL_SIZE / 2.0f, getHeight(cell) });
						verts.push_back(pos + Vec2{ 0.0f, getH(Vec2T{ x - 1, y }) });
					} else if (!left.isWall && left.density == 0 && !right.isWall && right.density == 0) {
						// Dividing y by 2 just for the visual effect.
						verts.push_back(pos + Vec2{ CELL_SIZE / 2.0f, getHeight(cell) / 2.0f });
					} else {
						if (!right.isWall && right.density == 0) {

						} else if (right.isWall || right.density < cell.density) {
							verts.push_back(pos + Vec2{ CELL_SIZE, getHeight(cell) });
						} else {
							verts.push_back(pos + Vec2{ CELL_SIZE, getH(Vec2T{ x + 1, y }) });
						}

						if (!left.isWall && left.density == 0) {

						} else if (left.isWall || left.density < cell.density) {
							verts.push_back(pos + Vec2{ 0.0f, getHeight(cell) });
						} else  {
							verts.push_back(pos + Vec2{ 0.0f, getH(Vec2T{ x - 1, y }) });
						}
					}
				} else {
					verts.push_back(pos + Vec2{ CELL_SIZE, CELL_SIZE });
					verts.push_back(pos + Vec2{ 0, CELL_SIZE });
				}

				if (showDensity) {
					const auto p = PixelRgba::scientificColoring(cell.density, 0, MAX_DENSITY);
					Debug::drawSimplePolygon(verts, Vec3(p.r, p.g, p.b) / 255.0f);
				} else {
					Debug::drawSimplePolygon(verts, Vec3{ 14, 135, 204 } / 255.0f);
				}
				
			}
		}
	}

	const auto cursorPos = Vec2T<i64>{ (camera.cursorPos() / CELL_SIZE).applied(floor) };
	if (cursorPos.x > 0 && cursorPos.y > 0 && cursorPos.x < GRID_SIZE.x - 1 && cursorPos.y < GRID_SIZE.y - 1) {
		auto& cell = cells[cursorPos.x][cursorPos.y];
		if (Input::isMouseButtonHeld(MouseButton::LEFT)) {
			cell.density = MAX_DENSITY;
		} else if (Input::isMouseButtonHeld(MouseButton::RIGHT)) {
			cell.isWall = true;
			cell.density = 0;
		} else if (Input::isMouseButtonHeld(MouseButton::MIDDLE)) {
			cell.isWall = false;
			cell.density = 0;
		} else if (Input::isKeyDown(Keycode::A)) {
			selected = cursorPos;
		}
	}


	Begin("water");
	Checkbox("paused", &paused);
	singleStep = false;
	if (paused) {
		singleStep = Button("single step");
	}
	InputInt("density", &cells[selected.x][selected.y].density);
	if (Button("reset")) {
		reset();
	}
	Checkbox("drawGrid", &drawGrid);
	Checkbox("showDensity", &showDensity);
	End();

	renderer.update(gfx, camera);
}

auto WaterDemo::reset() -> void {
	for (i64 x = 0; x < GRID_SIZE.x; x++) {
		for (i64 y = 0; y < GRID_SIZE.y; y++) {
			auto& cell = cells[x][y];
			cell.density = 0;
			cell.isWall = x == 0 || x == GRID_SIZE.x - 1 || y == 0 || y == GRID_SIZE.y - 1;
		}
	}
}
