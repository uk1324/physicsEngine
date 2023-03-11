#pragma once

#include <engine/renderer.hpp>

struct GravityInDifferentMetricsDemo {
	GravityInDifferentMetricsDemo();
	auto update() -> void;

	Camera camera;

	struct Body {
		Vec2 pos;
		float radius;
		float mass;
		Vec2 vel{ 0.0f };
		std::vector<Vec2> trial;
	};
	std::vector<Body> bodies;

	float selectedRadius = 0.1f;
	float selectedMass = 1.0f;
	float gravitationalConstant = 1.0f;
	Vec2 initialVelocity{ 0.0f };

	std::optional<Vec2> selectedPos;
};