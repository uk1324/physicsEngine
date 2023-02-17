#pragma once

#include <game/demo.hpp>
#include <math/vec2.hpp>

class CycloidDemo final : public Demo {
public:
	auto name() -> const char* override;
	auto loadSettingsGui() -> void override;
	auto load() -> void override;
	auto settingsGui() -> void;
	auto update() -> void override;

	auto posOnCycloid(float t) const -> Vec2;

	float cycloidRadius = 15.0f;

	bool showCircle = true;
	float circleT = 0.0f;
};
