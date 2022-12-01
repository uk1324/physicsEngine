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
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>

#include <chrono>

// TODO: Could make Noita like physics.

// TODO: When doing serizliation also create a type without the unserializable fields that could be used inside the editor.

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

static auto setCustomImGuiStyle() -> void {
	auto& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("assets/fonts/RobotoMono-Regular.ttf", 20);
	auto& style = ImGui::GetStyle();
	style.WindowRounding = 5.0f;
};

auto WINAPI WinMain( _In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR , _In_ int) -> int {
	Window::init("game", Vec2(640, 480));
	Gfx gfx{ Window::hWnd() };
	Audio::init();
	Game game{ gfx };

	ImGui::CreateContext();
	auto& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui_ImplWin32_Init(Window::hWnd());
	ImGui_ImplDX11_Init(gfx.device.Get(), gfx.ctx.Get());
	setCustomImGuiStyle();

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

			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();			

			// Without this the window doesn't close instantly.
			// TODO: Maybe find a better way to do this. The current way still has to wait untill the frame finishes.
			if (!Window::running())
				goto exit;

			Time::update(FRAME_TIME);
			// If the rendering is the bottleneck it might be better to take it out of this loop so the game can catch up be updating multiple times.

			Debug::update();
			game.update(gfx);

			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
			gfx.present();
		}
	}

	exit:
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	Window::destroy();
	return Window::exitCode();
}