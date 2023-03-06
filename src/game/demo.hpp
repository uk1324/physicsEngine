#pragma once

class Demo{
public:
	virtual ~Demo() = default;
	virtual auto name() -> const char* = 0;
	virtual auto loadSettingsGui() -> void {};
	virtual auto load() -> void {};
	virtual auto settingsGui() -> void {};
	virtual auto update() -> void {};
	virtual auto physicsStep() -> void {};
};