#include <pixelGames/eulerianFluid.hpp>
#include <engine/time.hpp>
#include <engine/input.hpp>
#include <engine/window.hpp>
#include <math/utils.hpp>
#include <math/aabb.hpp>
#include <game/debug.hpp>

#include <imgui/imgui.h>
using namespace ImGui;

Fluid::Fluid(Vec2T<i64> gridSize, float cellSpacing, float overRelaxation, float density)
	: gridSize{ gridSize }
	, cellSpacing{ cellSpacing }
	, density{ density }
	, overRelaxation{ overRelaxation } {
	const auto cellCount = gridSize.x * gridSize.y;
	velX.resize(cellCount);
	velY.resize(cellCount);
	newVelX.resize(cellCount);
	newVelY.resize(cellCount);
	pressure.resize(cellCount);
	smoke.resize(cellCount, 1.0f);
	isWallValues.resize(cellCount);
	newSmoke.resize(cellCount);

	// Place border walls.
	for (i64 x = 0; x < gridSize.x; x++) {
		for (i64 y = 0; y < gridSize.y; y++) {
			bool wall = false;
			if (x == 0 || x == gridSize.x - 1 || y == 0 || y == gridSize.y - 1)
				wall = true;
			setIsWall(x, y, wall);
		}
	}
}

auto Fluid::integrate(float dt, float gravity) -> void {
	for (i64 x = 1; x < gridSize.x; x++) {
		for (i64 y = 1; y < gridSize.y - 1; y++) {
			if (isWall(x, y) || isWall(x, y - 1))
				continue;
			at(velY, x, y) += gravity * dt;
		}
	}
}

auto Fluid::solveIncompressibility(i32 solverIterations, float dt) -> void {
	const auto pressureWithoutVelocity = density * (cellSpacing * cellSpacing) / dt;
	// If a fluid is incompressible it has to have a divergence of 0 at each point. The amount of fluid going out of a point has to be equal to the amount goint in. Solve using projection gauss seidel. To find approximate the global solution solve each cell separately multiple times.
	// Incompressible fluids are a good approximation of water.
	for (i64 iter = 0; iter < solverIterations; iter++) {

		for (i64 x = 1; x < gridSize.x - 1; x++) {
			for (i64 y = 1; y < gridSize.y - 1; y++) {
				if (isWall(x, y))
					continue;

				// std::vector<bool> is really slow in debug mode.
				const auto
					sx0 = !isWall(x - 1, y),
					sx1 = !isWall(x + 1, y),
					sy0 = !isWall(x, y - 1),
					sy1 = !isWall(x, y + 1);
				const auto outflowingSidesCount = sx0 + sx1 + sy0 + sy1;

				if (outflowingSidesCount == 0.0)
					continue;

				const auto divergence =
					- at(velX, x, y)
					+ at(velX, x + 1, y)
					- at(velY, x, y)
					+ at(velY, x, y + 1);

				// Outflow to each surrouding cell evenly.
				const auto correctedOutflow = (-divergence / outflowingSidesCount) * overRelaxation;
				at(pressure, x, y) += pressureWithoutVelocity * correctedOutflow;
				at(velX, x, y) -= sx0 * correctedOutflow;
				at(velX, x + 1, y) += sx1 * correctedOutflow;
				at(velY, x, y) -= sy0 * correctedOutflow;
				at(velY, x, y + 1) += sy1 * correctedOutflow;
			}
		}
	}
}

auto Fluid::sampleField(Vec2 pos, FieldType type) -> float {
	pos.x = std::clamp(pos.x, cellSpacing, gridSize.x * cellSpacing);
	pos.y = std::clamp(pos.y, cellSpacing, gridSize.y * cellSpacing);

	std::vector<float>* field = nullptr;

	Vec2 cellOffset{ 0.0f };
	switch (type) {
	case FieldType::VEL_X:
		field = &velX;
		cellOffset.y = cellSpacing / 2.0f;
		break;
	case FieldType::VEL_Y:
		field = &velY;
		cellOffset.x = cellSpacing / 2.0f;
		break;
	case FieldType::SMOKE:
		field = &smoke;
		cellOffset = Vec2{ cellSpacing / 2.0f };
		break;
	}

	// Could just use a single clamp here and remove the one from the top.
	const auto x0 = std::min(static_cast<i64>(floor((pos.x - cellOffset.x) / cellSpacing)), gridSize.x - 1);
	const auto tx = ((pos.x - cellOffset.x) - x0 * cellSpacing) / cellSpacing;
	const auto x1 = std::min(x0 + 1, gridSize.x - 1);

	const auto y0 = std::min(static_cast<i64>(floor((pos.y - cellOffset.y) / cellSpacing)), gridSize.y - 1);
	const auto ty = ((pos.y - cellOffset.y) - y0 * cellSpacing) / cellSpacing;
	const auto y1 = std::min(y0 + 1, gridSize.y - 1);

	const auto bilerpedValue =
		(1.0f - tx) * (1.0f - ty) * at(*field, x0, y0) +
		(1.0f - tx) * ty * at(*field, x0, y1) +
		tx * (1.0f - ty) * at(*field, x1, y0) +
		tx * ty * at(*field, x1, y1);

	return bilerpedValue;
}

auto Fluid::advectVelocity(float dt) -> void {
	newVelX = velX;
	newVelY = velY;

	for (i64 x = 1; x < gridSize.x; x++) {
		for (i64 y = 1; y < gridSize.y; y++) {
			if (isWall(x, y))
				continue;

			// Going back at step in a straight line and sampling the average previous pos to get the new velocity is called semi-lagrangian advection. This introduces viscosity.
			if (!isWall(x - 1, y) && y < gridSize.y - 1) {
				const auto pos = Vec2{ x + 0.0f, y + 0.5f } * cellSpacing;
				const auto avgVelY = (at(velY, x - 1, y) + at(velY, x, y) + at(velY, x - 1, y + 1) + at(velY, x, y + 1)) * 0.25f;
				const Vec2 vel{ at(velX, x, y), avgVelY };
				const auto approximatePreviousPos = pos - vel * dt;
				at(newVelX, x, y) = sampleField(approximatePreviousPos, FieldType::VEL_X);
			}

			if (!isWall(x, y - 1) && x < gridSize.x - 1) {
				const auto pos = Vec2{ x + 0.5f, y + 0.0f } * cellSpacing;
				const auto avgVelX = (at(velX, x, y - 1) + at(velX, x, y) + at(velX, x + 1, y - 1) + at(velX, x + 1, y)) * 0.25f;
				const Vec2 vel{ avgVelX, at(velY, x, y) };
				const auto approximatePreviousPos = pos - vel * dt;
				at(newVelY, x, y) = sampleField(approximatePreviousPos, FieldType::VEL_Y);
			}
		}
	}

	velX = newVelX;
	velY = newVelY;
}

auto Fluid::advectSmoke(float dt) -> void {
	newSmoke = smoke;

	for (i64 x = 1; x < gridSize.x - 1; x++) {
		for (i64 y = 1; y < gridSize.y - 1; y++) {
			if (isWall(x, y))
				continue;

			const auto avgVel = Vec2{ at(velX, x, y) + at(velX, x + 1, y), at(velY, x, y) + at(velY, x, y + 1) } / 2.0f;
			const auto pos = (Vec2{ Vec2T{ x, y } } + Vec2{ 0.5f }) * cellSpacing;
			const auto approximatePreviousPos = pos - dt * avgVel;
			at(newSmoke, x, y) = sampleField(approximatePreviousPos, FieldType::SMOKE);
		}
	}

	smoke = newSmoke;
}

auto Fluid::update(float dt, float gravity, i32 solverIterations) -> void {
	integrate(dt, gravity);
	fill(pressure.begin(), pressure.end(), 0.0f);
	solveIncompressibility(solverIterations, dt);
	advectVelocity(dt);
	advectSmoke(dt);
}

auto Fluid::setIsWall(i64 x, i64 y, bool value) -> void {
	isWallValues[x * gridSize.y + y] = value;
}

auto Fluid::isWall(i64 x, i64 y) const -> bool {
	return isWallValues[x * gridSize.y + y];
}

EulerianFluidDemo::EulerianFluidDemo(Gfx& gfx) 
	: texture{ gfx, GRID_SIZE }
	, fluid{ GRID_SIZE, 0.02f } {

	walls.resize(fluid.gridSize.x * fluid.gridSize.y, false);
}

auto EulerianFluidDemo::update(Gfx& gfx, Renderer& renderer) -> void {
	Camera camera;
	// TODO: Maybe move this to renderer update. The window size has to be passed anyway. Technically the window size only needs to be passed if it rendering to a texture so maybe make that an optional arugment.
	camera.aspectRatio = Window::aspectRatio();

	const auto cursorPos = camera.screenSpaceToCameraSpace(Input::cursorPos());
	const auto texturePos = Vec2{ 0.0f };
	auto textureHeight = camera.height();
	if (textureHeight * (texture.size().xOverY()) * camera.aspectRatio > camera.width()) {
		textureHeight = camera.width() / texture.size().xOverY();
	}
	auto textureSize = Vec2{ textureHeight * (texture.size().xOverY()), textureHeight };
	const auto textureBox = Aabb::fromPosSize(texturePos, Vec2{ texture.size() });

	auto fluidPosToCameraPos = [this, &textureSize](Vec2 pos) -> Vec2 {
		auto fluidGridSize = Vec2{ fluid.gridSize } * SPACE_BETWEEN_CELLS;
		return (pos / fluidGridSize - Vec2{ 0.5f }).flippedY() * textureSize;
	};

	if (textureBox.contains(cursorPos)) {
		auto gridPos = camera.posInGrid(cursorPos, texturePos, textureHeight, texture.size());

		//for (int y = 0; y < 5; y++) {
		//	fluid.at(fluid.velX, 2, fluid.gridSize.y / 2.0f + y) = 10.0f;
		//	fluid.at(fluid.smoke, 2, fluid.gridSize.y / 2.0f + y) = 0.0f;
		//}

		if (Input::isMouseButtonHeld(MouseButton::LEFT)) {
			Vec2 newPos = Vec2{ gridPos } * SPACE_BETWEEN_CELLS;
			auto vel = (newPos - obstaclePos) / Time::deltaTime();
			obstaclePos = newPos;

			elapsed += Time::deltaTime();

			// Skip borders
			for (i64 x = 1; x < fluid.gridSize.x - 2; x++) {
				for (i64 y = 1; y < fluid.gridSize.y - 2; y++) {
					const auto cellPos = Vec2{ Vec2T{ x, y } } * fluid.cellSpacing;
					const auto n = fluid.gridSize.y;
					if (!fluid.isWall(x, y) && (cellPos - obstaclePos).lengthSq() < pow(obstacleRadius, 2.0f)) {
						// Smoke doesn't impact velocities it only advectes (moves) with them. It is used to visualize how the fluid moves. The color doesn't represent the amount of fluid at a point. The fluid is incompressible so it has the same amount everywhere. The smoke is kind of like a fluid moving inside a fluid and it can vary from place to place. 

						// This code just sets the value of the smoke to a changing value to better show changes and it also looks cool.
						fluid.at(fluid.smoke, x, y) = 0.5f + 0.5f * sin(elapsed / Time::deltaTime() * 0.1f);
						fluid.at(fluid.velX, x, y) = vel.x;
						fluid.at(fluid.velX, x + 1, y) = vel.x;
						fluid.at(fluid.velY, x, y) = vel.y;
						fluid.at(fluid.velY, x, y + 1) = vel.y;
					}

				}
			}
		} 
		
		std::optional<bool> wallValue;

		if (Input::isMouseButtonHeld(MouseButton::RIGHT)) {
			wallValue = true;
		} else if (Input::isMouseButtonHeld(MouseButton::MIDDLE)) {
			wallValue = false;
		}

		if (wallValue.has_value()) {
			const auto radius = Vec2T<i64>{ 3 };
			const auto min = (gridPos - radius).clamped(Vec2T<i64>{ 0 }, fluid.gridSize - Vec2T<i64>{ 1 });
			const auto max = (gridPos + radius).clamped(Vec2T<i64>{ 0 }, fluid.gridSize - Vec2T<i64>{ 1 });
			for (i64 x = min.x; x < max.x; x++) {
				for (i64 y = min.y; y < max.y; y++) {
					walls[x * fluid.gridSize.y + y] = *wallValue;
				}
			}
		}

		for (i64 x = 1; x < fluid.gridSize.x - 1; x++) {
			for (i64 y = 1; y < fluid.gridSize.y - 1; y++) {
				fluid.setIsWall(x, y, walls[x * fluid.gridSize.y + y]);
			}
		}

		// This clears the velocities around walls. This is just a hack to prevent velocites going out from walls. It does create incorrect smoke value around walls, because it can't advect, which is pretty visible. Because no velocity advect out of the walls this creates neverending velocities that can only be removed by removing walls. A proper soultion would be to not sample values from walls. For this to be correct the averages would also need to be divided by the amount of walls. Removing only the velocitites asssocieted with the staggered grid cell might work, but I am not sure.
		for (i64 x = 0; x < fluid.gridSize.x; x++) {
			for (i64 y = 0; y < fluid.gridSize.y; y++) {
				if (fluid.isWall(x, y)) {
					fluid.at(fluid.velX, x, y) = 0.0f;
					fluid.at(fluid.velY, x, y) = 0.0f;
					if (x > 0) {
						fluid.at(fluid.velX, x - 1, y) = 0.0f;
						fluid.at(fluid.velY, x - 1, y) = 0.0f;
						/*if (y < fluid.gridSize.y - 1) {
							fluid.at(fluid.velX, x - 1, y + 1) = 0.0f;
							fluid.at(fluid.velY, x - 1, y + 1) = 0.0f;
						}*/
					}
						
					if (y > 0) {
						fluid.at(fluid.velX, x, y - 1) = 0.0f;
						fluid.at(fluid.velY, x, y - 1) = 0.0f;
		/*				if (x < fluid.gridSize.x - 1) {
							fluid.at(fluid.velX, x + 1, y - 1) = 0.0f;
							fluid.at(fluid.velY, x + 1, y - 1) = 0.0f;
						}*/
					}
						

					if (x < fluid.gridSize.x - 1) {
						fluid.at(fluid.velX, x + 1, y) = 0.0f;
						fluid.at(fluid.velY, x + 1, y) = 0.0f;
					}

					if (y < fluid.gridSize.y - 1) {
						fluid.at(fluid.velX, x, y + 1) = 0.0f;
						fluid.at(fluid.velY, x, y + 1) = 0.0f;
					}
				}
			}
		}
	}

	Begin("fluid simulation");
	TextWrapped("Use left to move the obstacle from the previous position, right to place a wall and middle to remove a wall");
	Checkbox("paused", &paused);
	Combo("draw", reinterpret_cast<int*>(&draw), "pressure\0smoke\0\0");
	Combo("coloring", reinterpret_cast<int*>(&coloring), "black and white\0scientific\0hsv\0\0");
	Checkbox("draw streamlines", &drawStreamlines);
	SliderFloat("streamline step size", &streamlineStepSize, 0.001f, 0.01f);
	InputFloat("gravity", &gravity);
	TextWrapped("Use lower values for over-relaxation and solver iterations for paint like effect. This produces invalid pressure.");
	SliderFloat("over-relaxation", &fluid.overRelaxation, 1.0f, 2.0f);
	SliderInt("solver iterations", &solverIterations, 10, 40);
	Checkbox("obstacle", &obstacle);
	if (obstacle) {
		SliderFloat("obstacle radius", &obstacleRadius, 0.05f, 1.0f);
	}
	if (Button("reset")) {
		fluid = Fluid{ fluid.gridSize, fluid.cellSpacing, fluid.overRelaxation, fluid.density };
		std::fill(walls.begin(), walls.end(), false);
		obstaclePos = Vec2{ 0.0f };
	}
	End();

	// TODO: A point (x, y) in camera space is the point(-x, -y) in the simulation. Fix this.
	if (!paused) {
		fluid.update(Time::deltaTime(), -gravity, solverIterations);
	}

	auto minPressure = std::numeric_limits<float>::infinity(), maxPressure = -std::numeric_limits<float>::infinity();
	for (auto& p : fluid.pressure) {
		minPressure = std::min(minPressure, p);
		maxPressure = std::max(maxPressure, p);
	}

	for (auto& p : texture.indexed()) {
		if (fluid.isWall(p.pos.x, p.pos.y)) {
			p = PixelRgba{ 128 };
			continue;
		}

		float value01 = 0.0f;
		if (draw == Draw::PRESSURE) {
			value01 = (fluid.at(fluid.pressure, p.pos.x, p.pos.y) - minPressure) / (maxPressure - minPressure);
		} else if (draw == Draw::SMOKE) {
			value01 = fluid.at(fluid.smoke, p.pos.x, p.pos.y);
		}

		switch (coloring) {
		case Coloring::BLACK_AND_WHITE: p = PixelRgba{ static_cast<u8>(value01 * 255.0f) }; break;
		case Coloring::SCIENTIFIC: p = PixelRgba::scientificColoring(value01, 0.0f, 1.0f); break;
		case Coloring::HSV: p = PixelRgba::fromHsv(value01, 1.0f, 1.0f); break;
		}

	}

	if (Input::isKeyHeld(Keycode::K)) {
		camera.zoom /= 3.0f;
	}

	Debug::drawAabb(Aabb::fromPosSize(texturePos, textureSize));

	if (drawStreamlines) {
		for (i64 x = 2; x < GRID_SIZE.x; x += 5) {
			for (i64 y = 2; y < GRID_SIZE.y; y += 5) {
				auto pos = (Vec2{ Vec2T{ x, y } } + Vec2{ 0.5f }) * SPACE_BETWEEN_CELLS;
				auto fullSize = Vec2{ fluid.gridSize } * SPACE_BETWEEN_CELLS;

				for (i32 i = 0; i < 15; i++) {
					const Vec2 vel{ fluid.sampleField(pos, Fluid::FieldType::VEL_X), fluid.sampleField(pos, Fluid::FieldType::VEL_Y) };
					const auto oldPos = pos;
					pos += vel * streamlineStepSize;
					/*Debug::drawLine((oldPos / fullSize - Vec2{ 0.5f }).flippedY() * textureSize, (pos / fullSize - Vec2{ 0.5f }).flippedY() * textureSize);*/
					Debug::drawLine(fluidPosToCameraPos(oldPos), fluidPosToCameraPos(pos));
				}
			}
		}
	}

	renderer.drawDynamicTexture(texturePos, textureHeight, texture, true);
	renderer.update(gfx, camera, Window::size(), false);
}
