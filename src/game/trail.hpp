#pragma once

#include <game/body.hpp>
#include <math/vec3.hpp>

#include <deque>

// TODO: Optimize allocation of the arrays. Use pooling for the types. Use a reset method instead of a destructor.
struct Trail {
	BodyId body;
	Vec2 anchor = Vec2{ 0.0f };
	Vec3 color = Vec3::RED;
	int maxHistorySize = 200;
	std::deque<Vec2> history;

	auto update() -> void;
	auto draw() const -> void;
};

using TrailId = EntityArray<Trail>::Id;