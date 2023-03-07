#pragma once

#include <engine/renderer.hpp>

struct DemoData {
	const Camera& camera;
	Vec2 cursorPos;
};

class Demo{
public:
	virtual ~Demo() = default;
	virtual auto name() -> const char* = 0;
	virtual auto loadSettingsGui() -> void {};
	virtual auto load() -> void {};
	virtual auto settingsGui() -> void {};
	virtual auto update(const DemoData& data) -> void {};
	virtual auto physicsStep() -> void {};
};