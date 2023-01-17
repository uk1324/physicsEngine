#include <demos/flipFluid.hpp>
#include <engine/window.hpp>
#include <engine/time.hpp>
#include <engine/input.hpp>

FlipFluid::FlipFluid(Vec2T<i64> gridSize, float cellSize, float overRelaxation)
	: gridSize{ gridSize }
	, cellSize{ cellSize }
	, overRelaxation{ overRelaxation } {

	const auto cellCount = gridSize.x * gridSize.y;
	cellTypes.resize(cellCount);
	velX.resize(cellCount);
	velY.resize(cellCount);

	for (i64 x = 0; x < gridSize.x; x++) {
		for (i64 y = 0; y < gridSize.y; y++) {
			auto& type = at(cellTypes, x, y);
			if (x == 0 || x == gridSize.x - 1 || y == 0 || y == gridSize.y - 1) {
				type = CellType::WALL;
			} else {
				type = CellType::AIR;
				if (y < gridSize.y / 1.1f && x < gridSize.x / 3.0f) {
					particles.push_back(Particle{ Vec2{ Vec2T{ x, y } } * cellSize, Vec2{ 0.0f } });
				}
			}
		}
	}
}

auto FlipFluid::integrate(float dt, float gravity) -> void {
	for (auto& particle : particles) {
		// Semi implicit euler.
		particle.vel.y += gravity * dt;
		particle.pos += particle.vel * dt;
		if (particle.pos.x < 1.5f * cellSize) {
			particle.pos.x = 1.5f * cellSize;
		}
		if (particle.pos.y < 1.5f * cellSize) {
			particle.pos.y = 1.5f * cellSize;
			particle.vel.y = 0.0f;
		}

		if (particle.pos.x > (gridSize.x - 2.5f) * cellSize) {
			particle.pos.x = (gridSize.x - 2.5f) * cellSize;
		}

		if (particle.pos.y > (gridSize.y - 2.5f) * cellSize) {
			particle.pos.y = (gridSize.y - 2.5f) * cellSize;
		}
	}
}

#include <math/utils.hpp>

auto FlipFluid::transferVelocititesFromParticlesToGrid() -> void {
	for (i64 x = 0; x < gridSize.x; x++) {
		for (i64 y = 0; y < gridSize.y; y++) {
			at(velX, x, y) = 0.0f;
			at(velY, x, y) = 0.0f;
			auto& type = at(cellTypes, x, y);
			if (type != CellType::WALL) {
				type = CellType::AIR;
			}
		}
	}

	std::vector<i32> c;
	c.resize(gridSize.x * gridSize.y, 0);

	for (const auto& particle : particles) {
		const auto gridPos = Vec2T<i64>{ (particle.pos / cellSize).applied(floor) };
		/*at(velX, gridPos.x, gridPos.y) += bilerp(velX, particle.pos);
		at(velY, gridPos.x, gridPos.y) += bilerp(velY, particle.pos);*/
		at(velX, gridPos.x, gridPos.y) += particle.vel.x;
		at(velY, gridPos.x, gridPos.y) += particle.vel.y;

		at(c, gridPos.x, gridPos.y) += 1;

		auto& type = at(cellTypes, gridPos.x, gridPos.y);
		if (type != CellType::WALL) {
			type = CellType::FLUID;
		}
	}

	for (usize i = 0; i < c.size(); i++) {
		if (c[i] != 0) {
			velX[i] /= c[i];
			velY[i] /= c[i];
		}
	}
}

auto FlipFluid::transferVelocitiesFromGridToParticles() -> void {

	auto bilerp = [this](std::vector<float>& v, Vec2 pos) -> float {
		const auto p = (pos / cellSize).applied(floor);
		const auto [x, y] = Vec2T<i64>{ p };
		auto t = pos - p * cellSize;
		float v00 = at(v, x, y);
		float v01 = at(v, x, y + 1);
		float v10 = at(v, x + 1, y);
		float v11 = at(v, x + 1, y + 1);

		float v0001 = lerp(v00, v01, t.y);
		float v1011 = lerp(v10, v11, t.y);

		return lerp(v0001, v1011, t.x);
	};

	for (auto& particle : particles) {
		const auto gridPos = Vec2T<i64>{ (particle.pos / cellSize).applied(floor) };
		/*particle.vel = Vec2{ at(velX, gridPos.x, gridPos.y), at(velY, gridPos.x, gridPos.y) };*/
		particle.vel = Vec2{ bilerp(velX, particle.pos), bilerp(velY, particle.pos) };
	}
}

auto FlipFluid::solveIncompressibility(i32 solverIterations) -> void {
	for (i64 iter = 0; iter < solverIterations; iter++) {

		for (i64 x = 1; x < gridSize.x - 2; x++) {
			for (i64 y = 1; y < gridSize.y - 2; y++) {
				if (at(cellTypes, x, y) != CellType::FLUID)
					continue;

				const auto
					sy1 = at(cellTypes, x, y + 1) == CellType::FLUID,
					sy0 = at(cellTypes, x, y - 1) == CellType::FLUID,
					sx1 = at(cellTypes, x + 1, y) == CellType::FLUID,
					sx0 = at(cellTypes, x - 1, y) == CellType::FLUID;
				const auto outflowingSidesCount = sx0 + sx1 + sy0 + sy1;

				if (outflowingSidesCount == 0)
					continue;

				const auto divergence =
					+ sx1 * at(velX, x + 1, y)
					- sx0 * at(velX, x - 1, y)
					+ sy1 * at(velY, x, y + 1)
					- sy0 * at(velY, x, y - 1);

				// Doesn't use this cell only the other ones.
				const auto correctedOutflow = (divergence / outflowingSidesCount) * overRelaxation;
				at(velX, x - 1, y) += sx0 * correctedOutflow;
				at(velX, x + 1, y) -= sx1 * correctedOutflow;
				at(velY, x, y - 1) += sy0 * correctedOutflow;
				at(velY, x, y + 1) -= sy1 * correctedOutflow;
			}
		}
	}
}

auto FlipFluid::update(float dt, float gravity, i32 incompressibilitySolverIterations) -> void {
	integrate(dt, -10.0f);
	for (int i = 0; i < 5; i++) {
		for (auto& a : particles) {
			for (auto& b : particles) {
				if (&a != &b) {
					if (distance(a.pos, b.pos) < cellSize / 2.0f) {
						a.pos += (a.pos - b.pos) / 2.0f;
						b.pos -= (a.pos - b.pos) / 2.0f;
					}
				}
			}
		}
	}
	transferVelocititesFromParticlesToGrid();
	solveIncompressibility(40);
	transferVelocitiesFromGridToParticles();
}

FlipFluidDemo::FlipFluidDemo(Gfx& gfx)
	: fluid{ GRID_SIZE, CELL_SIZE }
	, texture{ gfx, GRID_SIZE } {
}

#include <game/debug.hpp>

auto FlipFluidDemo::update(Gfx& gfx, Renderer& renderer) -> void {
	Camera camera;
	const auto fluidSize = Vec2{ fluid.gridSize } * fluid.cellSize;
	camera.pos = fluidSize / 2.0f;
	camera.zoom = 2.0f / fluidSize.x; // TODO: Put in fucntion set width.
	camera.aspectRatio = Window::aspectRatio();


	fluid.at(fluid.cellTypes, 1, 1) = FlipFluid::CellType::WALL;

	fluid.update(Time::deltaTime(), -1.0f, 10);
	// TODO: !!!!!!!!!!!!!!!! Update the camera aspedct ratio automatically.
	for (auto p : texture.indexed()) {
		const auto pos = Vec2T{ p.pos.x, fluid.gridSize.y - 1 - p.pos.y };
		const auto cellTypes = fluid.at(fluid.cellTypes, pos.x, pos.y);
		/*Debug::drawRay(Vec2{ pos } * fluid.cellSize, Vec2{ fluid.at(fluid.velX, pos.x, pos.y), fluid.at(fluid.velY, pos.x, pos.y) } * fluid.cellSize);*/
		if (cellTypes == FlipFluid::CellType::WALL) {
			p = PixelRgba{ 128 };
		} else if (cellTypes == FlipFluid::CellType::FLUID) {
			p = PixelRgba{ 0, 0, 128 };
		} else {
			p = PixelRgba{ 0, 0, 0 };
		}
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		const auto pos = camera.screenSpaceToCameraSpace(Input::cursorPos());
		fluid.particles.push_back(FlipFluid::Particle{ pos, Vec2{ 0.0f } });
	}

	for (auto& particle : fluid.particles) {
		Debug::drawCircle(particle.pos, fluid.cellSize / 2.0f);
		if (Input::isMouseButtonDown(MouseButton::RIGHT)) {
			particle.vel += Vec2{ 1.0f, 1.0f };
		}
	}

	renderer.drawDynamicTexture(camera.pos, fluidSize.y, texture);
	renderer.update(gfx, camera);
}
