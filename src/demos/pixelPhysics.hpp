#pragma once

#include <gfx/gfx.hpp>
#include <game/renderer.hpp>

#include <bitset>

// https://tomforsyth1000.github.io/papers/cellular_automata_for_physical_modelling.html
// https://www.youtube.com/watch?v=VLZjd_Y1gJ8
// https://www.youtube.com/@Winterdev/videos
// On issue that arises when making a celular automaton is in which order should the particles update. For example if paticles update top to bottom then a particle can move down multiple cells at a time. 
// Store if the particle was already updated this frame and don't update it if that is true. I tried this first, but there still were issues with it. I don't remember exactly what, but I think it was some pattern, maybe something like the pattern in powerdertoy when sand falls. Would need to experiment more with this approach. I use something similar to make sure that plants don't grow multiple times in a frame. This approach seems flexible for simulations that can move particles multiple cells in one frame.
// Iterate in different orders and directions for different types of cells. I choose this one, because it can be used to create different types of patterns for different particles. The directions of iteration (left or right, top or bottom) can also be randomized to prevent patterns like sand always falling in the same direction or water tending to go on direction over the other. Randomizing directions isn't exclusive to this approch and can be used with the others.
// Winterdev uses some sort of transaction system for his simulation in https://www.youtube.com/watch?v=wZJCQQPaGZI. Haven't looked into it much.

// Could make a system where particles have a density. Based on the velocity could orient in which direction the particles is facing. For example triangle it is facing down and left. Based on the shape it would also change how much water flows in that direction. This might create water that looks similar to the one in minecraft, but also conserves density.
// TODO: Drawing polygons. Triangulation?

// Making particles move faster. 
// Update faster or slower.
// Based on the material have the particles' different moves differnet have velocites. For example make water move left and right faster than lava. To make the lava appear more viscous. Example in video "Recreating Noita's Sand Simulation in C and OpenGL | Game Engineering". This also requires doing raycasts to prevent tunnelling.
// If the is a rigidbody is inside the pixel simulation noita just converts the pixels into particles outside of the simulation and converts them back to pixels when they hit another pixel. This is decribed in their GDC talk.

// TODO: Heat diffusion.
// Could make fire lose energy faster when it is surrounded by not air, because it doesn't have access to oxygen.

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
		PLANT,
		FLOWER,
		FIRE,
		WOOD,
		TERMITE,
		COUNT,
	};
	static constexpr auto BLOCK_TYPE_COUNT = static_cast<int>(Block::COUNT);
	
	// Block updated this frame, 
	// saving the old state and using it for updates (Still need to check both the old and the new state to not delete matter).
	// When updating columns first it looks really bad. It even looks like the particles are moving upward. 
	// When updating rows first there are some weird flows of particles also, but they don't look as weird as the column ones.

	i64 frameNumber = 0;
	Block blocks[GRID_SIZE.x][GRID_SIZE.y];
	bool grewThisFrame[GRID_SIZE.x][GRID_SIZE.y];
	decltype(blocks) oldBlocks;
	u8 fireFuelLeft[GRID_SIZE.x][GRID_SIZE.y];
	bool termiteUpdatedThisFrame[GRID_SIZE.x][GRID_SIZE.y];
	static constexpr u8 FIRE_MAX = 32;
	auto blockAt(Vec2T<i64> pos) -> Block&;
	Camera camera;

	bool paused = false;
	bool doSingleStep = false;
	i64 radius = 1;
	Block selectedBlock = Block::SAND;
	int fireSpreadChance = 3;
};

// Noita takes on collision the pixels out of the simulation and simulates them like particles to create splashing effects. What about using vector fields to simulate everything together.
// Fire

// Incompressisble (nieœciciliwy)
// Viscocity(lepkoœæ) is the friction of fluid

// Maybe create a virtual class for demos so some things are done automatically like setup and rendering.