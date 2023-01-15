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

#define U_FIELD  0
#define V_FIELD  1
#define S_FIELD  2
using namespace std;

Fluid::Fluid(double _density, int _numX, int _numY, double _h, double _overRelaxation, int _numThreads)
{
	density = _density;
	numX = _numX;
	numY = _numY;
	numCells = numX * numY;
	h = _h;
	u.resize(numCells);
	v.resize(numCells);
	newU.resize(numCells);
	newV.resize(numCells);
	Vel.resize(numCells);
	pressure.resize(numCells);
	s.resize(numCells);
	m.resize(numCells, 1.0);
	isWall.resize(numCells);
	newM.resize(numCells);
	num = numX * numY;
	overRelaxation = _overRelaxation;

	for (auto i = 0; i < this->numX; i++) {
		for (auto j = 0; j < this->numY; j++) {
			auto s = 1.0;	// fluid
			if (i == 0 || i == this->numX - 1 || j == 0 || j == this->numY - 1)
				s = 0.0;	// solid
			this->s[i * this->numY + j] = s;
		}
	}
}

void Fluid::integrate(double dt, double gravity)
{
	int n = numY;
	//#pragma omp parallel for schedule(static) num_threads(numThreads)
	for (int i = 1; i < numX; i++)
	{
		for (int j = 1; j < numY - 1; j++)
		{
			if (s[i * n + j] != 0.0 && s[i * n + j - 1] != 0.0)
				//#pragma omp atomic update
				v[i * n + j] += gravity * dt;
		}
	}
}

void Fluid::solveIncompressibility(int numIters, double dt)
{
	int n = numY;
	double cp = density * h / dt;
	for (int iter = 0; iter < numIters; iter++)
	{
		//#pragma omp parallel for schedule(static) num_threads(numThreads)
		for (int i = 1; i < numX - 1; i++)
		{
			for (int j = 1; j < numY - 1; j++)
			{
				if (s[i * n + j] == 0.0)
					continue;

				//                double s_ = s[i * n + j];
				double sx0 = s[(i - 1) * n + j];
				double sx1 = s[(i + 1) * n + j];
				double sy0 = s[i * n + j - 1];
				double sy1 = s[i * n + j + 1];
				double _s = sx0 + sx1 + sy0 + sy1;

				if (_s == 0.0)
					continue;

				double div = u[(i + 1) * n + j] - u[i * n + j] + v[i * n + j + 1] - v[i * n + j];

				double _p = -div / _s;
				_p *= overRelaxation;
				pressure[i * n + j] += cp * _p;
				u[i * n + j] -= sx0 * _p;
				u[(i + 1) * n + j] += sx1 * _p;
				v[i * n + j] -= sy0 * _p;
				v[i * n + j + 1] += sy1 * _p;
			}
		}
	}
}

void Fluid::extrapolate()
{
	int n = numY;
	for (int i = 0; i < numX; i++)
	{
		u[i * n + 0] = u[i * n + 1];
		u[i * n + numY - 1] = u[i * n + numY - 2];
	}
	for (int j = 0; j < numY; j++)
	{
		v[0 * n + j] = v[1 * n + j];
		v[(numX - 1) * n + j] = v[(numX - 2) * n + j];
	}
}

double Fluid::sampleField(double x, double y, int field)
{
	int n = numY;
	double h1 = 1.0 / h;
	double h2 = 0.5 * h;

	x = fmax(fmin(x, numX * h), h);
	y = fmax(fmin(y, numY * h), h);

	double dx = 0.0;
	double dy = 0.0;

	vector<double> f;

	switch (field)
	{
	case U_FIELD:
		f = u;
		dy = h2;
		break;
	case V_FIELD:
		f = v;
		dx = h2;
		break;
	case S_FIELD:
		f = m;
		dx = h2;
		dy = h2;
		break;
	}

	double x0 = fmin(floor((x - dx) * h1), numX - 1);
	double tx = ((x - dx) - x0 * h) * h1;
	double x1 = fmin(x0 + 1, numX - 1);

	double y0 = fmin(floor((y - dy) * h1), numY - 1);
	double ty = ((y - dy) - y0 * h) * h1;
	double y1 = fmin(y0 + 1, numY - 1);

	double sx = 1.0 - tx;
	double sy = 1.0 - ty;

	double val = sx * sy * f[x0 * n + y0] +
		tx * sy * f[x1 * n + y0] +
		tx * ty * f[x1 * n + y1] +
		sx * ty * f[x0 * n + y1];

	return val;
}

double Fluid::avgU(int i, int j)
{
	int n = numY;
	return (u[i * n + j - 1] + u[i * n + j] +
		u[(i + 1) * n + j - 1] + u[(i + 1) * n + j]) * 0.25;
}

double Fluid::avgV(int i, int j)
{
	int n = numY;
	return (v[(i - 1) * n + j] + v[i * n + j] +
		v[(i - 1) * n + j + 1] + v[i * n + j + 1]) * 0.25;
}

void Fluid::advectVelocity(double dt)
{
	newU = u;
	newV = v;

	int n = numY;
	double h2 = 0.5 * h;
	for (int i = 1; i < numX; i++)
	{
		for (int j = 1; j < numY; j++)
		{
			if (s[i * n + j] != 0.0 && s[(i - 1) * n + j] != 0.0 && j < numY - 1)
			{
				double x = i * h;
				double y = j * h + h2;
				double _u = u[i * n + j];
				double _v = avgV(i, j);

				//#pragma omp atomic update
				x -= dt * _u;
				//#pragma omp atomic update
				y -= dt * _v;
				_u = sampleField(x, y, U_FIELD);
				newU[i * n + j] = _u;
			}
			// v component
			if (s[i * n + j] != 0.0 && s[i * n + j - 1] != 0.0 && i < numX - 1)
			{
				double x = i * h + h2;
				double y = j * h;
				double _u = avgU(i, j);
				double _v = v[i * n + j];
				//#pragma omp atomic update
				x -= dt * _u;
				//#pragma omp atomic update
				y -= dt * _v;
				_v = sampleField(x, y, V_FIELD);
				newV[i * n + j] = _v;
			}
		}
	}

	u = newU;
	v = newV;
}

void Fluid::advectTracer(double dt)
{
	newM = m;

	int n = numY;
	double h2 = 0.5 * h;
	//#pragma omp parallel for schedule(static) num_threads(numThreads)
	for (int i = 1; i < numX - 1; i++)
	{
		for (int j = 1; j < numY - 1; j++)
		{

			if (s[i * n + j] != 0.0)
			{
				double _u = (u[i * n + j] + u[(i + 1) * n + j]) * 0.5;
				double _v = (v[i * n + j] + v[i * n + j + 1]) * 0.5;
				double x = i * h + h2 - dt * _u;
				double y = j * h + h2 - dt * _v;

				newM[i * n + j] = sampleField(x, y, S_FIELD);
			}
		}
	}
	m = newM;
}

void Fluid::simulate(double dt, double gravity, int numIters)
{
	integrate(dt, gravity);
	fill(pressure.begin(), pressure.end(), 0.0);
	solveIncompressibility(numIters, dt);
	extrapolate();
	advectVelocity(dt);
	advectTracer(dt);
}

void Fluid::updateFluidParameters()
{
	numCells = numX * numY;
	u.resize(numCells);
	v.resize(numCells);
	newU.resize(numCells);
	newV.resize(numCells);
	pressure.resize(numCells);
	s.resize(numCells);
	m.resize(numCells, 1.0);
	newM.resize(numCells);
	num = numX * numY;
}

static auto staggeredGridBilerp(float x00, float x01, float x10, float x11, float tx, float ty) -> float {
	return
		(1.0f - tx) * (1.0f - ty) * x00 +
		(1.0f - tx) * ty * x01 +
		tx * (1.0f - tx) * x10 +
		tx * ty * x11;
}

struct StaggeredGridBilerpPositions {
	i64 x0, x1, y0, y1;
	float tx, ty;
};
static auto staggeredGridBilerpPositions(Vec2 pos, Vec2 offset, float spaceBetweenCells, Vec2T<i64> gridSize) -> StaggeredGridBilerpPositions {
	const auto x0 = std::clamp(static_cast<i64>(std::floor((pos.x - offset.x) / spaceBetweenCells)), 0ll, gridSize.x);
	const auto tx = ((pos.x - offset.x) - x0 * spaceBetweenCells) / spaceBetweenCells;
	const auto x1 = std::clamp(x0 + 1, 0ll, gridSize.x - 1);

	const auto y0 = std::clamp(static_cast<i64>(std::floor((pos.y - offset.y) / spaceBetweenCells)), 0ll, gridSize.y - 1);
	const auto ty = ((pos.y - offset.y) - y0 * spaceBetweenCells) / spaceBetweenCells;
	const auto y1 = std::clamp(y0 + 1, 0ll, gridSize.y - 1);

	return { x0, x1, y0, y1, tx, ty };
};

EulerianFluid::EulerianFluid(Gfx& gfx) 
	: texture{ gfx, GRID_SIZE }, fluid(1000.0f, GRID_SIZE.x, GRID_SIZE.y, 0.02f) {

	auto v = staggeredGridBilerpPositions(Vec2{ 0.030808858343710502f, 0.0407460988437136f }, Vec2{ 0.01f, 0.0f }, 0.02f, GRID_SIZE);

	for (i64 x = 0; x < GRID_SIZE.x; x++) {
		for (i64 y = 0; y < GRID_SIZE.y; y++) {
			//velocities[x][y] = Vec2::oriented(static_cast<float>(rand()) / RAND_MAX * TAU<float>) / 100000.0f;
			velocities[x][y] = Vec2{ 0.00000f };
		}
	}

	for (auto& col : smoke) {
		for (auto& v : col) {
			v = 1.0f;
		}
	}

	for (i64 x = 0; x < GRID_SIZE.x; x++) {
		setIsWall(x, 0, true);
		setIsWall(x, GRID_SIZE.y - 1, true);
	}

	for (i64 y = 1; y < GRID_SIZE.y - 1; y++) {
		setIsWall(0, y, true);
		setIsWall(GRID_SIZE.x - 1, y, true);
	}
}

#include <utils/io.hpp>

static auto getSciColor(float v, float minV, float maxV) -> PixelRgba {
	v = std::min(std::max(v, minV), maxV - 0.0001f);
	auto d = maxV - minV;
	v = d == 0.0f ? 0.5f : (v - minV) / d;
	auto m = 0.25f;
	int num = static_cast<int>(std::floor(v / m));
	auto s = (v - num * m) / m;
//float r, g, b;

// There is a bug and num can be outisde the switch range . Which causes uninitialized memory access.
float r = 0.0f, g = 0.0f, b = 0.0f;

switch (num) {
case 0: r = 0.0; g = s; b = 1.0; break;
case 1: r = 0.0; g = 1.0; b = 1.0 - s; break;
case 2: r = s; g = 1.0; b = 0.0; break;
case 3: r = 1.0; g = 1.0 - s; b = 0.0; break;
}

return { static_cast<u8>(255 * r), static_cast<u8>(255 * g), static_cast<u8>(255 * b) };
}

auto EulerianFluid::update(Gfx& gfx, Renderer& renderer) -> void {

	// Gauss seidel.

	// Modify velocities (gravity and other outside forces).

	for (i64 x = 1; x < GRID_SIZE.x - 1; x++) {
		for (i64 y = 1; y < GRID_SIZE.y - 1; y++) {
			if (isWall(x, y) || isWall(x, y - 1))
				continue;

			//const auto gravity = -0.98f;
			const auto gravity = 0.0f;
			velocities[x][y].y += gravity * Time::deltaTime();
		}
	}

	const auto overRelaxation = 1.9f;

	// Make the fluid incompressible (projection).
	// Solve incompresibility
	const auto solverIterations = 40;
	const auto density = 1000.0f;
	auto cp = density * SPACE_BETWEEN_CELLS / Time::deltaTime();

	for (auto& col : preassure) {
		for (auto& v : col) {
			v = 0.0f;
		}
	}

	//for (i32 i = 0; i < solverIterations; i++) {

	//	for (i64 x = 1; x < GRID_SIZE.x - 1; x++) {
	//		for (i64 y = 1; y < GRID_SIZE.y - 1; y++) {
	//			if (isWall(x, y))
	//				continue;

	//			/*
	//			The staggered grid of velocites around the cell [x, y] looks like this.
	//					vy[x, y + 1]
	//			vx[x, y]            vx[x + 1, y]
	//					  vy[x, y]
	//			*/

	//			const auto outflowingSidesCount =
	//				!isWall(x - 1, y) +
	//				!isWall(x + 1, y) +
	//				!isWall(x, y - 1) +
	//				!isWall(x, y + 1);

	//			if (outflowingSidesCount == 0)
	//				continue;

	//			// Total outflow. If outflow is negative then it is inflow.
	//			const auto divergence =
	//				- velocities[x][y].x
	//				+ velocities[x + 1][y].x
	//				- velocities[x][y].y
	//				+ velocities[x][y + 1].y;

	//			// An incompressible fluid has to have a divergence of zero.
	//			// A fluid has to flow equally in all directions.
	//			const auto outflow = -divergence / outflowingSidesCount * 1.9f;
	//			// The walls are the same as in outflowingSidesCount.
	//			preassure[x][y] += outflow * cp;
	//			velocities[x][y].x -= !isWall(x - 1, y) * outflow;
	//			velocities[x][y].y -= !isWall(x, y - 1) * outflow;
	//			velocities[x + 1][y].x += !isWall(x + 1, y) * outflow;
	//			velocities[x][y + 1].y += !isWall(x, y + 1) * outflow;
	//		}
	//	}
	//}

	// Move the velocity field advection.
	memcpy(oldVelocities, velocities, sizeof(oldVelocities));

	//for (i64 x = 1; x < GRID_SIZE.x; x++) {
	//	for (i64 y = 1; y < GRID_SIZE.y; y++) {
	//		if (!isWall(x, y) && !isWall(x - 1, y) && y < GRID_SIZE.y - 1) {
	//			const Vec2 pos{ x * SPACE_BETWEEN_CELLS, (y + 0.5f) * SPACE_BETWEEN_CELLS };
	//			const auto avgVelY = (oldVelocities[x - 1][y].y + oldVelocities[x][y].y + oldVelocities[x - 1][y + 1].y + oldVelocities[x][y + 1].y) / 4.0f;
	//			const Vec2 vel{ oldVelocities[x][y].x, avgVelY };
	//			const auto approximatePreviousPos = pos - vel * Time::deltaTime();
	//			const Vec2 offset{ 0.0f, SPACE_BETWEEN_CELLS / 2.0f };
	//			const auto p = staggeredGridBilerpPositions(approximatePreviousPos, offset, SPACE_BETWEEN_CELLS, GRID_SIZE);
	//			velocities[x][y].x = staggeredGridBilerp(oldVelocities[p.x0][p.y0].x, oldVelocities[p.x0][p.y1].x, oldVelocities[p.x1][p.y0].x, oldVelocities[p.x1][p.y1].x, p.tx, p.ty);
	//		}

	//		// TOOD: take out !isWall(x, y) contineu.
	//		if (!isWall(x, y) && !isWall(x, y - 1) && x < GRID_SIZE.x - 1) {
	//			const Vec2 pos{ (x + 0.5f) * SPACE_BETWEEN_CELLS, y * SPACE_BETWEEN_CELLS, };
	//			const auto avgVelX = (oldVelocities[x][y - 1].x + oldVelocities[x][y].x + oldVelocities[x + 1][y - 1].x + oldVelocities[x + 1][y].x) / 4.0f;
	//			const Vec2 vel{ avgVelX, oldVelocities[x][y].y };
	//			const auto approximatePreviousPos = pos - vel * Time::deltaTime();
	//			const Vec2 offset{ SPACE_BETWEEN_CELLS / 2.0f, 0.0f };
	//			const auto p = staggeredGridBilerpPositions(approximatePreviousPos, offset, SPACE_BETWEEN_CELLS, GRID_SIZE);
	//			velocities[x][y].y = staggeredGridBilerp(oldVelocities[p.x0][p.y0].y, oldVelocities[p.x0][p.y1].y, oldVelocities[p.x1][p.y0].y, oldVelocities[p.x1][p.y1].y, p.tx, p.ty);
	//		}
	//	}
	//}

	memcpy(oldSmoke, smoke, sizeof(smoke));

	//for (i64 x = 0; x < GRID_SIZE.x; x++) {
	//	for (i64 y = 0; y < GRID_SIZE.y; y++) {
	//		if (isWall(x, y))
	//			continue;

	//		auto u = (velocities[x][y].x + velocities[x + 1][y].x) * 0.5f;
	//		auto v = (velocities[x][y].y + velocities[x][y + 1].y) * 0.5f;
	//		auto xa = x * SPACE_BETWEEN_CELLS + SPACE_BETWEEN_CELLS / 2.0f - Time::deltaTime() * u;
	//		auto ya = y * SPACE_BETWEEN_CELLS + SPACE_BETWEEN_CELLS / 2.0f - Time::deltaTime() * v;

	//		const Vec2 offset{ SPACE_BETWEEN_CELLS / 2.0f};
	//		const auto p = staggeredGridBilerpPositions(Vec2{ xa, ya }, offset, SPACE_BETWEEN_CELLS, GRID_SIZE);
	//		smoke[x][y] = staggeredGridBilerp(oldSmoke[p.x0][p.y0], oldSmoke[p.x0][p.y1], oldSmoke[p.x1][p.y0], oldSmoke[p.x1][p.y1], p.tx, p.ty);
	//	}
	//}

	//auto minPressure = std::numeric_limits<float>::infinity(), maxPressure = -std::numeric_limits<float>::infinity();
	//for (auto& col : preassure) {
	//	for (auto& v : col) {
	//		minPressure = std::min(minPressure, v);
	//		maxPressure = std::max(maxPressure, v);
	//	}
	//}

	//for (auto& p : texture.indexed()) {
	//	const auto pos = GRID_SIZE - Vec2T<i64>{ 1 } - p.pos;
	//	if (isWall(pos.x, pos.y)) {
	//		p = PixelRgba{ 128 };
	//	} else {
	//		const auto v = preassure[pos.x][pos.y];
	//		p = getSciColor(v, minPressure, maxPressure);
	//	}
	//}

	//for (auto& p : texture.indexed()) {
	//	const auto pos = GRID_SIZE - Vec2T<i64>{ 1 } - p.pos;
	//	if (isWall(pos.x, pos.y)) {
	//		p = PixelRgba{ 128 };
	//	} else {
	//		const auto v = smoke[pos.x][pos.y];
	//		//p = getSciColor(v, 0.0f, 1.0f);
	//		p = PixelRgba{ static_cast<u8>(v * 255.0f) };
	//	}
	//}

	/*for (auto& p : texture.indexed()) {
		const auto pos = GRID_SIZE - Vec2T<i64>{ 1 } - p.pos;
		if (isWall(pos.x, pos.y)) {
			p = PixelRgba{ 128 };
		} else {
			const auto v = preassure[pos.x][pos.y];
			p = getSciColor(v, minPressure, maxPressure);
		}
	}*/

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
		gridPos = texture.size() - gridPos;
		/*velocities[gridPos.x][gridPos.y] = Vec2::oriented(static_cast<float>(rand()) / RAND_MAX * TAU<float>);*/
		velocities[gridPos.x][gridPos.y] = Vec2{ 0.0f, 1.0f };
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 5; j++) {
				fluid.u[(gridPos.x + j) * fluid.numY + gridPos.y + i] = 5.0f;
				fluid.m[(gridPos.x + j) * fluid.numY + gridPos.y + i] = 0.0f;
			}
		}
		//setIsWall(gridPos.x, gridPos.y, true);
	}

	if (Input::isKeyHeld(Keycode::K)) {
		camera.zoom /= 6.01f;
	}

	fluid.simulate(Time::deltaTime(), -9.0f, 40);
	auto min = std::numeric_limits<double>::infinity(), max = -std::numeric_limits<double>::infinity();
	for (auto& p : fluid.pressure) {
		min = std::min(min, p);
		max = std::max(max, p);
	}
	for (auto& p : texture.indexed()) {
		if (fluid.s[p.pos.x * fluid.numY + p.pos.y] == 0.0f)
		{
			p = PixelRgba{ 128 };
			continue;
		}
		auto v = fluid.pressure[p.pos.x * fluid.numY + p.pos.y];
		p = getSciColor(v, min, max);
		if (p.data->r != 0) {
			int x = 5;
		}
			
	}

	for (auto& p : texture.indexed()) {
		const auto pos = GRID_SIZE - Vec2T<i64>{ 1 } - p.pos;
		if (isWall(pos.x, pos.y)) {
			p = PixelRgba{ 128 };
		} else {
			const auto v = fluid.m[p.pos.x * fluid.numY + p.pos.y];
			//p = getSciColor(v, 0.0f, 1.0f);
			p = PixelRgba{ static_cast<u8>(v * 255.0f) };
		}
	}

	for (i64 x = 0; x < GRID_SIZE.x; x += 5) {
		for (i64 y = 0; y < GRID_SIZE.y; y += 5) {
			auto pos = (Vec2{ Vec2T{ x, y } } + Vec2{ 0.5f }) * SPACE_BETWEEN_CELLS;
			for (i32 i = 0; i < 15; i++) {
				Vec2 v;
				{
					const Vec2 offset{ SPACE_BETWEEN_CELLS / 2.0f, 0.0f };
					const auto p = staggeredGridBilerpPositions(pos, offset, SPACE_BETWEEN_CELLS, GRID_SIZE);
					v.y = staggeredGridBilerp(velocities[p.x0][p.y0].y, velocities[p.x0][p.y1].y, velocities[p.x1][p.y0].y, velocities[p.x1][p.y1].y, p.tx, p.ty);
				}

				{
					const Vec2 offset{ 0.0f, SPACE_BETWEEN_CELLS / 2.0f };
					const auto p = staggeredGridBilerpPositions(pos, offset, SPACE_BETWEEN_CELLS, GRID_SIZE);
					v.x = staggeredGridBilerp(velocities[p.x0][p.y0].x, velocities[p.x0][p.y1].x, velocities[p.x1][p.y0].x, velocities[p.x1][p.y1].x, p.tx, p.ty);
				}
				v.x = fluid.sampleField(pos.x, pos.y, U_FIELD);
				v.y = fluid.sampleField(pos.x, pos.y, V_FIELD);

				//const Vec2T<i64> gridPos{ pos.clamped(Vec2{ 0 }, Vec2{ GRID_SIZE }.applied(floor)) };
				//const auto vel = velocities[gridPos.x][gridPos.y];
				const auto oldPos = pos;
				pos += v / 100.0f;
				Debug::drawLine(oldPos - size / 2.0f, pos - size / 2.0f, Vec3::RED);
			}
		}
	}

	renderer.drawDynamicTexture(texturePos, textureSize, texture);
	renderer.update(gfx, camera, Window::size(), false);
}

auto EulerianFluid::isWall(i64 x, i64 y) -> bool {
	ASSERT(x < GRID_SIZE.x);
	ASSERT(y < GRID_SIZE.y);
	return isWallBitset.test(x * GRID_SIZE.y + y);
}

auto EulerianFluid::setIsWall(i64 x, i64 y, bool value) -> void {
	ASSERT(x < GRID_SIZE.x);
	ASSERT(y < GRID_SIZE.y);
	isWallBitset.set(x * GRID_SIZE.y + y, value);
}
