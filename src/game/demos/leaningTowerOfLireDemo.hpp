#pragma once

#include <game/demo.hpp>

class LeaningTowerOfLireDemo final : public Demo {
public:
	auto name() -> const char* override;
	auto loadSettingsGui() -> void override;
	auto load() -> void override;

	int height = 13;
	float blockWidth = 10.0f;
	float blockHeight = 1.0f;
	float moveBlocksBack = 0.75f;
};
