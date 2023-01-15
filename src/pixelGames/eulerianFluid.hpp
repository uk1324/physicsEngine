#pragma once

#include <gfx/gfx.hpp>
#include <game/renderer.hpp>
#include <math/vec2.hpp>

#include <bitset>


class Fluid {
public:
	Fluid(Vec2T<i64> gridSize, float cellSpacing, float overRelaxation = 1.9f, float density = 1000.0f);

	auto integrate(float dt, float gravity) -> void;
	auto solveIncompressibility(i32 solverIterations, float dt) -> void;
	auto extrapolate() -> void;
	enum class FieldType {
		VEL_X, VEL_Y, SMOKE
	};
	auto sampleField(Vec2 pos, FieldType type) -> float;
	void advectVelocity(float dt);
	void advectSmoke(float dt);
	void update(float dt, float gravity, i32 solverIterations);
	template<typename T>
	auto at(std::vector<T>& vec, i64 x, i64 y) -> T&;
	auto setIsWall(i64 x, i64 y, bool value) -> void;
	auto isWall(i64 x, i64 y) const -> bool;

	float density;
	Vec2T<i64> gridSize;
	float cellSpacing;
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
};

// https://www.youtube.com/watch?v=iKAVRgIrUOU
// Eulerian means grid based.
// Lagrangian methods are not grid based for example particles.

// Assuemes that the fluid is incompressible.
// Zero viscosity
struct EulerianFluid {
	Fluid fluid;
	EulerianFluid(Gfx& gfx);
	auto update(Gfx& gfx, Renderer& renderer) -> void;
	// Collocated grid vs staggered grid.
	// For fluid simulations a staggered grid is better.
	// +2 for walls
	static constexpr Vec2T<i64> GRID_SIZE{  2 * 96 + 2, 2 * 50 + 2 };
	static constexpr float SPACE_BETWEEN_CELLS = 0.02f;
	DynamicTexture texture;
	Vec2 obstaclePos{ 0.0f };

	enum class Draw {
		PRESSURE, SMOKE
	};

	bool drawStreamlines = false;
	bool useScientificColoring = true;
	Draw draw = Draw::SMOKE;
	float gravity = 0.0f;
};

template<typename T>
auto Fluid::at(std::vector<T>& vec, i64 x, i64 y) -> T& {
	return vec[x * gridSize.y + y];
}
