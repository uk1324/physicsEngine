#pragma once

#include <math/vec2.hpp>

#include <pch.hpp>

class Window {
public:
	static auto init(const char* title, Vec2 size) -> void;
	static auto destroy() -> void;

	static auto update() -> void;

	static auto hWnd() -> HWND { return hWnd_; };

	static auto size() -> const Vec2& { return size_; };
	static auto aspectRatio() -> float { return size_.x / size_.y; }
	static auto resized() -> bool { return resizedOnThisFrame; };

	static auto running() -> bool { return running_; };
	static auto exitCode() -> int { return exitCode_; };

	static auto maximize() -> void;

private:
	static auto WINAPI windowMessageCallback(HWND hWnd_, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT;

	static constexpr auto WINDOW_CLASS_NAME = "game";

	static HWND hWnd_;

	static Vec2 size_;
	static bool firstFrame;
	static bool resizedOnThisFrame;

	static bool running_;
	static int exitCode_;
};