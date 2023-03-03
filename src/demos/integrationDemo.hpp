#pragma once

#include <math/vec2.hpp>

struct IntegrationDemo {
	IntegrationDemo();
	auto update() -> void;
	struct Box {
		Vec2 pos;
		float velY = 0.0f;
	};
	Box boxes[6];
};