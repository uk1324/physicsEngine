#pragma once

#include <game/demo.hpp>

class PyramidDemo final : public Demo {
public:
	auto name() -> const char* override;
	auto loadSettingsGui() -> void override;
	auto load() -> void override;
	int pyramidHeight = 10;
	float boxSize = 1.0f;
};