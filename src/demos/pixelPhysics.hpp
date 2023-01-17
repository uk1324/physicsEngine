#pragma once

#include <gfx/gfx.hpp>
#include <game/renderer.hpp>

#include <bitset>

struct PixelPhysics {
	PixelPhysics(Gfx& gfx);

	auto update(Gfx& gfx, Renderer& renderer) -> void;

	DynamicTexture texture;
	static constexpr Vec2T<i64> GRID_SIZE{ 150, 100 };
	static constexpr float CELL_SIZE = 1.0f;
	enum class Block : u8 {
		AIR,
		SAND,
		WALL,
		WATER,
		COUNT,
	};
	static constexpr auto BLOCK_TYPE_COUNT = static_cast<int>(Block::COUNT);
	
	// Block updated this frame, 
	// saving the old state and using it for updates (Still need to check both the old and the new state to not delete matter).
	// When updating columns first it looks really bad. It even looks like the particles are moving upward. 
	// When updating rows first there are some weird flows of particles also, but they don't look as weird as the column ones.

	i64 frameNumber = 0;
	Block blocks[GRID_SIZE.x][GRID_SIZE.y];
	decltype(blocks) oldBlocks;
	auto blockAt(Vec2T<i64> pos) -> Block&;
	Camera camera;

	bool paused = false;
	bool doSingleStep = false;
	i64 radius = 1;
	Block selectedBlock = Block::SAND;
};

// Noita takes on collision the pixels out of the simulation and simulates them like particles to create splashing effects. What about using vector fields to simulate everything together.
// Fire

// Incompressisble (nieœciciliwy)
// Viscocity(lepkoœæ) is the friction of fluid

// Maybe create a virtual class for demos so some things are done automatically like setup and rendering.