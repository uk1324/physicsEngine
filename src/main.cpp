#include <pch.hpp>
#include <winUtils.hpp>
#include <gfx/gfx.hpp>
#include <game/game.hpp>
#include <engine/time.hpp>
#include <engine/window.hpp>
#include <engine/input.hpp>

#include <chrono>

// TODO: Could make something similar to rust's dbg macro by using templates. Using variadic templates probably wouldn't work because there is no way to stringify all the arguments in the macro and pass them to a function. Might be possible be creating multiple copies of the macro with different argument counts?

#include <utils/dbg.hpp>

auto WINAPI WinMain( _In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR , _In_ int) -> int {
	const auto x = dbg(5 + 2);
	return 0;
	Window::init("game", Vec2(640, 480));
	Gfx gfx{ Window::hWnd() };
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

			gfx.update();

			Input::update();
			Window::update();
			// Without this the window doesn't close instantly.
			// TODO: Maybe find a better way to do this. The current way still has to wait untill the frame finishes.
			if (!Window::running())
				goto exit;

			Time::update(FRAME_TIME);
			// If the rendering is the bottleneck it might be better to take it out of this loop so the game can catch up be updating multiple times.
			game.update(gfx);
			gfx.present();
		}
	}

	exit:
	Window::destroy();
	return Window::exitCode();
}