#pragma once

#include <engine/renderer.hpp>
#include <math/vec2.hpp>

#include <bitset>
#include <vector>

// Based on https://www.youtube.com/watch?v=iKAVRgIrUOU
// Eulerian means grid based.
class Fluid {
public:
	Fluid(Vec2T<i64> gridSize, float cellSpacing, float overRelaxation = 1.9f, float density = 1000.0f);

	auto integrate(float dt, float gravity) -> void;
	auto solveIncompressibility(i32 solverIterations, float dt) -> void;
	enum class FieldType {
		VEL_X, VEL_Y, SMOKE
	};
	auto sampleField(Vec2 pos, FieldType type) -> float;
	auto advectVelocity(float dt) -> void;
	auto advectSmoke(float dt) -> void;
	auto update(float dt, float gravity, i32 solverIterations) -> void;
	template<typename T>
	auto at(std::vector<T>& vec, i64 x, i64 y) -> T&;
	auto setIsWall(i64 x, i64 y, bool value) -> void;
	auto isWall(i64 x, i64 y) const -> bool;

	float density;
	Vec2T<i64> gridSize;
	float cellSpacing;
	// Speeds up convergence https://en.wikipedia.org/wiki/Successive_over-relaxation.
	float overRelaxation;
	// @Performance: Could double buffer the velocities instead of copying them. Would need to profile to see if the extra level of indirection has any impact on performance.
	std::vector<float> velX;
	std::vector<float> velY;
	std::vector<float> newVelX;
	std::vector<float> newVelY;
	std::vector<float> pressure;
	std::vector<bool> isWallValues;
	std::vector<float> smoke;
	std::vector<float> newSmoke;
	std::vector<float> divergence;
	/*
	Velocities v around a point [x, y] lie on a staggered grid.
	        v0[x, y + 1]
	v0[x, y]            v0[x + 1, y]
	          v0[x, y]
	The cells are cellSpacing apart from eachother.
	To get the velocity at the point [x, y] these values have to be interpolated.
	Other values don't need to be interpolated.
	*/
};

// TODO: Fluid simulation with diffusion: https://damassets.autodesk.net/content/dam/autodesk/research/publications-assets/pdf/realtime-fluid-dynamics-for.pdf
struct EulerianFluidDemo {
	Fluid fluid;
	EulerianFluidDemo();
	auto update() -> void;
	static constexpr Vec2T<i64> GRID_SIZE{ 2 * 96 + 2, 2 * 50 + 2 };
	static constexpr float SPACE_BETWEEN_CELLS = 0.02f;
	DynamicTexture texture;
	Vec2 obstaclePos{ 0.0f };

	// Save walls in a seperate array to make it easier to easier to erase the previous obstacle walls by erasing all the walls and then just copy this into the fluid walls.
	std::vector<bool> walls;

	float elapsed = 0.0f;

	struct VelocityGenerator {
		Vec2T<i64> gridPos;
		Vec2 velocity;
		i64 radius;
		float spinSpeed;
	};
	std::vector<VelocityGenerator> velocityGenerators;

	bool drawStreamlines = false;
	float streamlineStepSize = 0.01f;
	enum class Draw {
		PRESSURE, SMOKE
	};
	Draw draw = Draw::SMOKE;
	enum class Coloring {
		BLACK_AND_WHITE,
		SCIENTIFIC,
		HSV
	};
	Coloring coloring = Coloring::SCIENTIFIC;
	i32 solverIterations = 40;
	float gravity = 0.0f;
	bool paused = false;
	bool obstacle = true;
	float obstacleRadius = 0.25f;
};

template<typename T>
auto Fluid::at(std::vector<T>& vec, i64 x, i64 y) -> T& {
	return vec[x * gridSize.y + y];
}
