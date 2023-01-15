#include <pixelGames/eulerianFluid.hpp>
#include <engine/time.hpp>
#include <engine/input.hpp>
#include <engine/window.hpp>
#include <math/utils.hpp>
#include <math/aabb.hpp>
#include <game/debug.hpp>

#include <imgui/imgui.h>
using namespace ImGui;

#include <memory>
#include <vector>
#include <math.h>
#include <algorithm>

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
	smoke.resize(cellCount, 1.0);
	isWallValues.resize(cellCount);
	newSmoke.resize(cellCount);

	for (auto x = 0; x < gridSize.x; x++) {
		for (auto y = 0; y < gridSize.y; y++) {
			bool wall = false;
			if (x == 0 || x == gridSize.x - 1 || y == 0 || y == gridSize.y - 1)
				wall = true;
			setIsWall(x, y, wall);
		}
	}
}

void Fluid::integrate(float dt, float gravity) {
	for (i64 x = 1; x < gridSize.x; x++) {
		for (i64 y = 1; y < gridSize.y - 1; y++) {
			if (isWall(x, y) || isWall(x, y - 1))
				continue;
			at(velY, x, y) += gravity * dt;
		}
	}
}

void Fluid::solveIncompressibility(i32 numIters, float dt) {
	double cp = density * cellSpacing / dt;
	for (i64 iter = 0; iter < numIters; iter++) {

		for (i64 x = 1; x < gridSize.x - 1; x++) {
			for (i64 y = 1; y < gridSize.y - 1; y++) {
				if (isWall(x, y))
					continue;

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

				const auto correctedOutflow = (-divergence / outflowingSidesCount) * overRelaxation;
				at(pressure, x, y) += cp * correctedOutflow;
				at(velX, x, y) -= sx0 * correctedOutflow;
				at(velX, x + 1, y) += sx1 * correctedOutflow;
				at(velY, x, y) -= sy0 * correctedOutflow;
				at(velY, x, y + 1) += sy1 * correctedOutflow;
			}
		}
	}
}

void Fluid::extrapolate()
{
	for (int i = 0; i < gridSize.x; i++)
	{
		velX[i * gridSize.y + 0] = velX[i * gridSize.y + 1];
		velX[i * gridSize.y + gridSize.y - 1] = velX[i * gridSize.y + gridSize.y - 2];
	}
	for (int j = 0; j < gridSize.y; j++)
	{
		velY[0 * gridSize.y + j] = velY[1 * gridSize.y + j];
		velY[(gridSize.x - 1) * gridSize.y + j] = velY[(gridSize.x - 2) * gridSize.y + j];
	}
}

float Fluid::sampleField(Vec2 pos, FieldType type) {
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
void Fluid::advectVelocity(float dt)
{
	//int n = gridSize.y;
	//double h2 = 0.5 * h;
	//for (int i = 1; i < gridSize.x; i++)
	//{
	//	for (int j = 1; j < gridSize.y; j++)
	//	{
	//		if (s[i * n + j] != 0.0 && s[(i - 1) * n + j] != 0.0 && j < gridSize.y - 1)
	//		{
	//			double x = i * h;
	//			double y = j * h + h2;
	//			double _u = u[i * n + j];
	//			double _v = avgV(i, j);

	//			x -= dt * _u;
	//			y -= dt * _v;
	//			_u = sampleField(x, y, FieldType::VEL_X);
	//			newU[i * n + j] = _u;
	//		}
	//		// v component
	//		if (s[i * n + j] != 0.0 && s[i * n + j - 1] != 0.0 && i < gridSize.x - 1)
	//		{
	//			double x = i * h + h2;
	//			double y = j * h;
	//			double _u = avgU(i, j);
	//			double _v = v[i * n + j];
	//			x -= dt * _u;
	//			y -= dt * _v;
	//			_v = sampleField(x, y, FieldType::VEL_Y);
	//			newV[i * n + j] = _v;
	//		}
	//	}
	//}

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
				const auto approximatePreviousPos = pos - vel * Time::deltaTime();
				at(newVelX, x, y) = sampleField(approximatePreviousPos, FieldType::VEL_X);
			}

			if (!isWall(x, y - 1) && x < gridSize.x - 1) {
				const auto pos = Vec2{ x + 0.5f, y + 0.0f } * cellSpacing;
				const auto avgVelX = (at(velX, x, y - 1) + at(velX, x, y) + at(velX, x + 1, y - 1) + at(velX, x + 1, y)) * 0.25f;
				const Vec2 vel{ avgVelX, at(velY, x, y) };
				const auto approximatePreviousPos = pos - vel * Time::deltaTime();
				at(newVelY, x, y) = sampleField(approximatePreviousPos, FieldType::VEL_Y);
			}
		}
	}

	velX = newVelX;
	velY = newVelY;
}

void Fluid::advectSmoke(float dt) {
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

void Fluid::update(float dt, float gravity, i32 solverIterations)
{
	integrate(dt, gravity);
	fill(pressure.begin(), pressure.end(), 0.0);
	solveIncompressibility(solverIterations, dt);
	extrapolate();
	advectVelocity(dt);
	advectSmoke(dt);
}

auto Fluid::setIsWall(i64 x, i64 y, bool value) -> void {
	isWallValues[x * gridSize.y + y] = value;
}

auto Fluid::isWall(i64 x, i64 y) const -> bool {
	return isWallValues[x * gridSize.y + y];
}

EulerianFluid::EulerianFluid(Gfx& gfx) 
	: texture{ gfx, GRID_SIZE }
	, fluid{ GRID_SIZE, 0.02f } {}

auto EulerianFluid::update(Gfx& gfx, Renderer& renderer) -> void {

	// Gauss seidel.

	// Modify velocities (gravity and other outside forces).


	// Make the fluid incompressible (projection).
	// Solve incompresibility


	
	Camera camera;
	// TODO: Maybe move this to renderer update. The window size has to be passed anyway. Technically the window size only needs to be passed if it rendering to a texture so maybe make that an optional arugment.
	camera.aspectRatio = Window::aspectRatio();

	const auto cursorPos = camera.screenSpaceToCameraSpace(Input::cursorPos());
	const auto texturePos = Vec2{ 0.0f };
	auto textureSize = camera.height();
	auto size = Vec2{ textureSize * (texture.size().xOverY()), textureSize };
	if (textureSize * (texture.size().xOverY()) * camera.aspectRatio > camera.width()) {
		textureSize = camera.width() / texture.size().xOverY();
	}
	const auto textureBox = Aabb::fromPosSize(texturePos, Vec2{ texture.size() });
	if (textureBox.contains(cursorPos) && Input::isMouseButtonHeld(MouseButton::LEFT)) {
		auto gridPos = camera.posInGrid(cursorPos, texturePos, textureSize, texture.size());

		Vec2 pos = Vec2{ gridPos } * SPACE_BETWEEN_CELLS;

		auto vx = 0.0;
		auto vy = 0.0;

		vx = (pos.x - obstaclePos.x) / Time::deltaTime();
		vy = (pos.y - obstaclePos.y) / Time::deltaTime();

		obstaclePos.x = pos.x;
		obstaclePos.y = pos.y;
		double r = 0.15f;
		float cd = sqrt(2) * 0.02f;

		static double elapsed = 0.0f;
		elapsed += Time::deltaTime();

		for (auto i = 1; i < fluid.gridSize.x - 2; i++) {
			for (auto j = 1; j < fluid.gridSize.y - 2; j++) {

				fluid.setIsWall(i, j, false);
				//fluid.s[i * fluid.gridSize.y + j] = 1.0;

				auto dx = (i + 0.5) * 0.02f - pos.x;
				auto dy = (j + 0.5) * 0.02f - pos.y;
				const auto n = fluid.gridSize.y;
				if (dx * dx + dy * dy < r * r) {
					//fluid.s[i * n + j] = 0.0;
					fluid.setIsWall(i, j, true);
					// if (scene.sceneNr == 2) 
					fluid.smoke[i * n + j] = 0.5 + 0.5 * sin(elapsed / Time::deltaTime() * 0.1f);
						// else 
						// 	f.m[i*n + j] = 1.0;
					fluid.velX[i * n + j] = vx;
					fluid.velX[(i + 1) * n + j] = vx;
					fluid.velY[i * n + j] = vy;
					fluid.velY[i * n + j + 1] = vy;
				}
			}
		}
	}



	Begin("test");
	auto item = static_cast<int>(draw);
	Combo("draw", &item, "pressure\0smoke\0\0");
	draw = static_cast<Draw>(item);
	Checkbox("use scientific coloring", &useScientificColoring);
	InputFloat("gravity", &gravity);
	End();


	fluid.update(Time::deltaTime(), gravity, 40);



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

		float value = 0.0f, minValue = 0.0f, maxValue = 0.0f;
		if (draw == Draw::PRESSURE) {
			value = fluid.at(fluid.pressure, p.pos.x, p.pos.y);
			minValue = minPressure;
			maxValue = maxPressure;
		} else if (draw == Draw::SMOKE) {
			value = fluid.at(fluid.smoke, p.pos.x, p.pos.y);
			minValue = 0.0f;
			maxValue = 1.0f;
		}

		p = useScientificColoring
			? PixelRgba::scientificColoring(value, minValue, maxValue)
			: PixelRgba{ static_cast<u8>(value * 255.0f) };
	}

	//if (false) {
	//	for (auto& p : texture.indexed()) {
	//		if (fluid.isWall(p.pos.x, p.pos.y))
	//		{
	//			p = PixelRgba{ 128 };
	//			continue;
	//		}
	//		auto v = fluid.pressure[p.pos.x * fluid.gridSize.y + p.pos.y];
	//		p = PixelRgba::scientificColoring(v, minPressure, maxPressure);

	//	}
	//} else {
	//	for (auto& p : texture.indexed()) {
	//		const auto pos = GRID_SIZE - Vec2T<i64>{ 1 } - p.pos;

	//		if (fluid.isWall(p.pos.x, p.pos.y)) {
	//			p = PixelRgba{ 128 };
	//		} else {
	//			const auto v = fluid.smoke[p.pos.x * fluid.gridSize.y + p.pos.y];
	//			p = PixelRgba::scientificColoring(v, 0.0f, 1.0f);
	//			//p = PixelRgba{ static_cast<u8>(v * 255.0f) };
	//		}
	//	}
	//}

	for (i64 x = 0; x < GRID_SIZE.x; x += 5) {
		for (i64 y = 0; y < GRID_SIZE.y; y += 5) {
			auto pos = (Vec2{ Vec2T{ x, y } } + Vec2{ 0.5f }) * SPACE_BETWEEN_CELLS;
			for (i32 i = 0; i < 15; i++) {
				const Vec2 vel{ fluid.sampleField(pos, Fluid::FieldType::VEL_X), fluid.sampleField(pos, Fluid::FieldType::VEL_Y) };
				const auto oldPos = pos;
				pos += vel / 100.0f;
				Debug::drawLine(oldPos - size / 2.0f, pos - size / 2.0f, Vec3::BLACK);
			}
		}
	}

	renderer.drawDynamicTexture(texturePos, textureSize, texture);
	renderer.update(gfx, camera, Window::size(), false);
}
