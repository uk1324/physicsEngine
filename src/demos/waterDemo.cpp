#include <demos/waterDemo.hpp>
#include <game/debug.hpp>
#include <engine/time.hpp>
#include <game/input.hpp>

#include <imgui/imgui.h>
using namespace ImGui;

WaterDemo::WaterDemo() {
	for (i64 x = 0; x < GRID_SIZE.x ; x++) {
		for (i64 y = 0; y < GRID_SIZE.y; y++) {
			if (x == 0 || x == GRID_SIZE.x - 1 || y == 0 || y == GRID_SIZE.y - 1)
				cells[x][y].isWall = true;
		}
	}
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
		for (i64 x = 1; x < GRID_SIZE.x - 1; x++) {
			for (i64 y = 1; y < GRID_SIZE.y - 1; y++) {
			/*for (i64 y = GRID_SIZE.y - 2; y > 1; y--) {*/
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
				if (cell.density <= 1)
					continue;

				auto& left = cells[x - 1][y];
				auto& right = cells[x + 1][y];

				if ((left.density > 0 && cell.density - left.density == 1) && (right.density > 0 && cell.density - right.density == 1))
					continue;

				if (!right.isWall && right.density < cell.density && !left.isWall && left.density < cell.density) {
					const auto t = cell.density / 3;
					const auto l = cell.density % 3;
					cell.density /= 3;
					cell.density += l;


					left.density += t;
					const auto overl = std::max(0, left.density - MAX_DENSITY);
					left.density -= overl;
					cell.density += overl; 
					auto o = left.density - cell.density;
					//if (o > 0) {
					//	left.density -= o;
					//	cell.density += o;
					//}

					right.density += t;
					const auto overr = std::max(0, right.density - MAX_DENSITY);
					right.density -= overr;
					cell.density += overr;
					//o = right.density - cell.density;
					//if (o > 0) {
					//	right.density -= o;
					//	cell.density += o;
					//}

				} else if (!right.isWall && right.density < cell.density) {
					/*if (right.density > 0 && cell.density - right.density == 1)
						continue;*/

					right.density += cell.density / 2;
					auto old = cell.density;
					cell.density /= 2;
					if (old % 2 == 1) {
						cell.density += 1;
					}

					const auto over = std::max(0, right.density - MAX_DENSITY);
					right.density -= over;
					cell.density += over;

					auto o = right.density - cell.density;
					if (o > 0) {
						right.density -= o;
						cell.density += o;
					}
				} else if (!left.isWall && left.density < cell.density) {
					/*if (left.density > 0 && cell.density - left.density == 1)
						continue;*/

					left.density += cell.density / 2;
					auto old = cell.density;
					cell.density /= 2;
					if (old % 2 == 1) {
						cell.density += 1;
					}

					const auto over = std::max(0, left.density - MAX_DENSITY);
					left.density -= over;
					cell.density += over;

					auto o = left.density - cell.density;
					if (o > 0) {
						left.density -= o;
						cell.density += o;
					}
					
				}
				//auto& below = cells[x][y - 1];
				//if (!below.isWall) {
				//	below.density += cell.density;
				//	const auto over = std::max(0, below.density - MAX_DENSITY);
				//	below.density -= over;
				//	cell.density = over;
				//}
				//if (cell.density <= 1)
				//	continue;

				//auto& left = cells[x - 1][y];
				//auto& right = cells[x + 1][y];
				//auto l = [&]() {
				//	if (!left.isWall && left.density < cell.density) {
				//		left.density += cell.density / 2;
				//		auto old = cell.density;
				//		cell.density /= 2;
				//		if (old % 2 == 1) {
				//			cell.density += 1;
				//		}

				//		const auto over = std::max(0, left.density - MAX_DENSITY);
				//		left.density -= over;
				//		cell.density += over;
				//	}
				//};
				//auto r = [&]() {
				//	if (!right.isWall && right.density < cell.density) {
				//		right.density += cell.density / 2;
				//		auto old = cell.density;
				//		cell.density /= 2;
				//		if (old % 2 == 1) {
				//			cell.density += 1;
				//		}

				//		const auto over = std::max(0, right.density - MAX_DENSITY);
				//		right.density -= over;
				//		cell.density += over;
				//	};
				//};

				//r();
				//l();

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
				//const Vec2 verts[] = {
				//	pos,
				//	pos + Vec2{ 0, CELL_SIZE },
				//	pos + Vec2{ CELL_SIZE, CELL_SIZE },
				//	pos + Vec2{ CELL_SIZE, 0 }
				//};
				//Debug::drawSimplePolygon(verts, Vec3{ 14,135,204 } / 255.0f);

				//const auto pos = (Vec2{ Vec2T{ x, y } } + Vec2{ 0.0f, cell.bottomY }) * CELL_SIZE;
				const auto height = cell.density;
				std::vector<Vec2> verts;
				verts.push_back(pos);
				verts.push_back(pos + Vec2{ CELL_SIZE, 0 });

				auto getHeight = [](const Cell& cell) -> float {
					return cell.density / static_cast<float>(MAX_DENSITY) * CELL_SIZE;
				};

				auto& right = cells[x + 1][y];
				auto& left = cells[x - 1][y];
				auto& belowRight = cells[x + 1][y - 1];
				auto& belowLeft = cells[x - 1][y - 1];
				if (cells[x][y + 1].density == 0) {
					if (!right.isWall && right.density > cell.density && !left.isWall && left.density > cell.density) {
						verts.push_back(pos + Vec2{ CELL_SIZE, getHeight(right) });
						verts.push_back(pos + Vec2{ CELL_SIZE / 2.0f, getHeight(cell) });
						verts.push_back(pos + Vec2{ 0.0f, getHeight(left) });
					} else if (!left.isWall && left.density > cell.density) {
						verts.push_back(pos + Vec2{ CELL_SIZE, getHeight(cell) });
						verts.push_back(pos + Vec2{ 0.0f, getHeight(left) });
					} else if (!right.isWall && right.density > cell.density) {
						verts.push_back(pos + Vec2{ CELL_SIZE, getHeight(right) });
						verts.push_back(pos + Vec2{ 0.0f, getHeight(cell) });
					} else if (left.density == 0 && right.density == 0) {
						verts.push_back(pos + Vec2{ CELL_SIZE / 2.0f, getHeight(cell) });
					} else if (left.density == 0) {
						if (!right.isWall && right.density > cell.density) {
							verts.push_back(pos + Vec2{ CELL_SIZE, getHeight(right) });
						} else {
							verts.push_back(pos + Vec2{ CELL_SIZE, getHeight(cell) });
						}
					} else if (right.density == 0) {
						if (!left.isWall && right.density > cell.density) {
							verts.push_back(pos + Vec2{ 0.0f, getHeight(left) });
						} else {
							verts.push_back(pos + Vec2{ 0.0f, getHeight(cell) });
						}
					} else {
						verts.push_back(pos + Vec2{ CELL_SIZE, getHeight(cell) });
						verts.push_back(pos + Vec2{ 0.0f, getHeight(cell) });
					}
				} else {
					verts.push_back(pos + Vec2{ CELL_SIZE, CELL_SIZE });
					verts.push_back(pos + Vec2{ 0, CELL_SIZE });
				}

				/*Debug::drawSimplePolygon(verts, Vec3{ 14,135,204 } / 255.0f);*/
				/*auto p = PixelRgba::scientificColoring(cell.density, 0, 8);
				Debug::drawSimplePolygon(verts, Vec3(p.r, p.g, p.b) / 255.0f);*/
				Debug::drawSimplePolygon(verts, Vec3{ 14, 135, 204 } / 255.0f);
			}
		}
	}

	Debug::drawPoint(Vec2{ 0.0f });

	const auto cursorPos = Vec2T<i64>{ (camera.cursorPos() / CELL_SIZE).applied(floor) };
	if (Input::isMouseButtonHeld (MouseButton::LEFT)) {
		cells[cursorPos.x][cursorPos.y].density = MAX_DENSITY;
	}

	if (Input::isMouseButtonHeld(MouseButton::RIGHT)) {
		cells[cursorPos.x][cursorPos.y].isWall = true;
	}

	if (Input::isMouseButtonHeld(MouseButton::MIDDLE)) {
		cells[cursorPos.x][cursorPos.y].isWall = false;
	}

	Checkbox("paused", &paused);
	singleStep = false;
	if (paused) {
		singleStep = Button("single step");
	}

	renderer.update(gfx, camera);
}
