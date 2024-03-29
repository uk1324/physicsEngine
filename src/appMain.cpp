#include <appMain.hpp>
#include <game/gameMain.hpp>
#include <engine/time.hpp>
#include <engine/debug.hpp>
#include <engine/window.hpp>
#include <engine/audio.hpp>
#include <engine/input.hpp>
#include <engine/frameAllocator.hpp>
#include <imgui/imgui.h>
#include <utils/io.hpp>
#include <framework.hpp>

#include <chrono>

// TODO: Use an actual testing framework.
auto testAreaAllocator() -> void {
	AreaAllocator allocator{ 1024 * 1024 };
	// Allocate all
	{
		const auto m = reinterpret_cast<u8*>(allocator.allocAlligned(1, 1024 * 1024));
		m[1024 * 1024 - 1] = 'a';
	}

	allocator.reset();
	// Allocations changing pages
	{
		auto m = reinterpret_cast<u8*>(allocator.allocAlligned(1, 4096 * 2));
		*m = 'a';
		m = reinterpret_cast<u8*>(allocator.allocAlligned(1, 4096 * 2));
		*m = 'a';
	}

	// Allocations on the same page
	{
		auto m = reinterpret_cast<u8*>(allocator.allocAlligned(1, 1024));
		*m = 'a';
		m = reinterpret_cast<u8*>(allocator.allocAlligned(1, 1024));
		*m = 'a';
	}
}



auto appMain() -> int {
	Framework::init();
	// Uses too much stack memory.
	const auto gameMain = std::make_unique<GameMain>();

	// Remember to use a high precision timestamp so the numbers don't get rounded down to 0.
	using namespace std::chrono_literals;
	using namespace std::chrono;
	auto previousFrameStart = high_resolution_clock::now();

	auto accumulated = previousFrameStart - previousFrameStart;
	while (Window::running()) {
		auto frameLength = duration_cast<nanoseconds>(16670000ns / Time::timeScale);
		const auto frameStart = high_resolution_clock::now();
		auto elapsed = frameStart - previousFrameStart;
		if (elapsed > frameLength * 2) {
			elapsed = frameLength * 2;
		}
		accumulated += elapsed;
		previousFrameStart = frameStart;

		while (accumulated >= frameLength) {
			accumulated -= frameLength;
			frameAllocator.reset();
			// If the rendering is the bottleneck it might be better to take it out of this loop so the game can catch up be updating multiple times.
			Renderer::updateFrameStart();

			Input::update();
			Window::update();
			Audio::update();		

			// Without this the window doesn't close instantly.
			// TODO: Maybe find a better way to do this. The current way still has to wait untill the frame finishes.
			if (!Window::running())
				goto exit;

			// Don't update with the scaled frame time so the game is deterministic.
			static constexpr auto FRAME_TIME = 1.0f / 60.0f;
			const auto oldTimeScale = Time::timeScale;
			Time::update(FRAME_TIME);
			if (Time::timeScale != oldTimeScale)
				accumulated = 0ns;

			Debug::update();
			gameMain->update();

			// WARNING: When vsync is enabled the game loop can't update faster that 60hz.
			const auto enableVsync = true;
			Renderer::updateFrameEnd(enableVsync);
		}
	}

	exit:
	Renderer::terminate();
	ImGui::DestroyContext();
	Window::destroy();
	return Window::exitCode();
}