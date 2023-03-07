#pragma once

#include <game/demo.hpp>

class TestingDemo final : public Demo {
public:
	auto name() -> const char* override;
	auto loadSettingsGui() -> void override;
	auto load() -> void override;
	auto settingsGui() -> void override;
	auto update(const DemoData& data) -> void override;
	auto physicsStep() -> void override;
};
