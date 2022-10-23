#include <pch.hpp>
#include <winUtils.hpp>
#include <gfx/gfx.hpp>
#include <game/game.hpp>
#include <game/debug.hpp>
#include <engine/time.hpp>
#include <engine/window.hpp>
#include <engine/audio.hpp>
#include <engine/input.hpp>
#include <engine/frameAllocator.hpp>

#include <chrono>

// TODO: Could make Noita like physics.

// TODO: Integrate ImGui use custom vector type and using namespace Im = ImGui

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

auto WINAPI WinMain( _In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR , _In_ int) -> int {
	Window::init("game", Vec2(640, 480));
	Gfx gfx{ Window::hWnd() };
	Audio::init();
	Game game{ gfx };

	auto currentTime = []() -> float {
		using namespace std::chrono;
		return duration<float> { duration_cast<seconds>(high_resolution_clock::now().time_since_epoch()) }.count();
	};

	auto previousFrameStart{ currentTime() }, accumulated{ 0.0f };
	static constexpr auto FRAME_TIME = 1.0f / 60.0f;
	while (Window::running()) {
		const auto frameStart{ currentTime() };
		const auto elapsed{ frameStart - previousFrameStart };
		accumulated += elapsed;
		previousFrameStart = frameStart;

		while (accumulated >= FRAME_TIME) {
			accumulated -= FRAME_TIME;
			frameAllocator.reset();
			gfx.update();

			Input::update();
			Window::update();
			Audio::update();
			// Without this the window doesn't close instantly.
			// TODO: Maybe find a better way to do this. The current way still has to wait untill the frame finishes.
			if (!Window::running())
				goto exit;

			Time::update(FRAME_TIME);
			// If the rendering is the bottleneck it might be better to take it out of this loop so the game can catch up be updating multiple times.

			Debug::update();
			game.update(gfx);
			gfx.present();
		}
	}

	exit:
	Window::destroy();
	return Window::exitCode();
}