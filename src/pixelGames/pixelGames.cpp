#include <pixelGames/pixelGames.hpp>
#include <imgui/imgui.h>

PixelGames::PixelGames(Gfx& gfx)
	: fourierTransformDemo{ gfx } 
	, eulerianFluid{ gfx } {}


auto PixelGames::update(Gfx& gfx, Renderer& renderer) -> void {
	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_NoDockingInCentralNode | ImGuiDockNodeFlags_PassthruCentralNode);
	//fourierTransformDemo.update(gfx, renderer);
	eulerianFluid.update(gfx, renderer);
}
