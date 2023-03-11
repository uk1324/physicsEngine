#pragma once

#include <engine/renderer.hpp>

struct ImageToSdfDemo {
	ImageToSdfDemo();
	auto update() -> void;

	int frame = 100;
	bool paused = true;
	DynamicTexture texture;
	//std::bitset<360 * 480> bitset;
	bool bitset[360 * 480];
};