#include <framework.hpp>
#include <imgui/imgui.h>
#include <engine/window.hpp>
#include <engine/audio.hpp>
#include <engine/renderer.hpp>

static auto setCustomImGuiStyle() -> void {
	auto& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("assets/fonts/RobotoMono-Regular.ttf", 20);
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	auto& style = ImGui::GetStyle();
	style.WindowRounding = 5.0f;
};

auto Framework::init() -> void {
	// ImGui_ImplWin32_WndProcHandler is called inside the window proc handler.
	Window::init("game", Vec2(640, 480));
	Audio::init();
	ImGui::CreateContext();
	auto& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	setCustomImGuiStyle();
	Renderer::init();
}

auto Framework::terminate() -> int {
	return 0;
}
