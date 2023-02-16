#pragma once

#include <game/demo.hpp>

class DoubleDominoDemo final : public Demo {
public:
	auto name() -> const char* override;
	auto loadSettingsGui() -> void override;
	auto load() -> void override;
	int dominoCount = 10;
};