#pragma once

#include <gfx/gfx.hpp>
#include <game/renderer.hpp>
#include <math/vec2.hpp>

#include <vector>

struct FlipFluid {
	FlipFluid(Vec2T<i64> gridSize, float cellSize, float overRelaxation = 1.9f);

	auto integrate(float dt, float gravity) -> void;
	auto transferVelocititesFromParticlesToGrid() -> void;
	auto transferVelocitiesFromGridToParticles() -> void;
	auto solveIncompressibility(i32 solverIterations) -> void;
	auto update(float dt, float gravity, i32 incompressibilitySolverIterations) -> void;

	template<typename T>
	auto at(std::vector<T>& vec, i64 x, i64 y)-> T&;

	enum class CellType {
		WALL, AIR, FLUID
	};
	struct Particle {
		Vec2 pos, vel;
	};
	float overRelaxation;
	std::vector<CellType> cellTypes;
	std::vector<Particle> particles;
	std::vector<float> velX;
	std::vector<float> velY;
	Vec2T<i64> gridSize;
	float cellSize;
};

struct FlipFluidDemo {
	FlipFluidDemo(Gfx& gfx);

	auto update(Gfx& gfx, Renderer& renderer) -> void;

	static constexpr Vec2T<i64> GRID_SIZE{ 100, 50 };
	static constexpr float CELL_SIZE = 0.2f;
	DynamicTexture texture;
	FlipFluid fluid;
};

template<typename T>
auto FlipFluid::at(std::vector<T>& vec, i64 x, i64 y) -> T& {
	return vec[x * gridSize.y + y];
}

