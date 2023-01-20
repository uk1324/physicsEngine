#include <demos/demos.hpp>
#include <imgui/imgui.h>

Demos::Demos(Gfx& gfx)
	: fourierTransformDemo{ gfx } 
	, eulerianFluid{ gfx } 
	, pixelPhysics{ gfx } {}
	
auto Demos::update(Gfx& gfx, Renderer& renderer) -> void {
	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_NoDockingInCentralNode | ImGuiDockNodeFlags_PassthruCentralNode);
	//pixelPhysics.update(gfx, renderer);
	//triangulationDemo.update(gfx, renderer);
	//fourierTransformDemo.update(gfx, renderer);
	//eulerianFluid.update(gfx, renderer);
	waterDemo.update(gfx, renderer);
}
