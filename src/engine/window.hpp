#pragma once

#include <math/vec2.hpp>
#include <utils/imageRgba.hpp>

class Window {
public:
	static auto init(const char* title, Vec2 size) -> void;
	static auto destroy() -> void;

	static auto update() -> void;

	static auto hWnd() -> void*;

	static auto setSize(Vec2 size) -> void;
	static auto size() -> const Vec2&;
	static auto aspectRatio() -> float;
	static auto resized() -> bool;

	static auto running() -> bool;
	static auto exitCode() -> int;

	static auto maximize() -> void;
	static auto minimize() -> void;
};