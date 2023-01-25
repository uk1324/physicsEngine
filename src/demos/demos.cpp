#include <demos/demos.hpp>
#include <imgui/imgui.h>

auto Demos::update() -> void {
	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_NoDockingInCentralNode | ImGuiDockNodeFlags_PassthruCentralNode);
	//pixelPhysics.update();
	//triangulationDemo.update();
	//fourierTransformDemo.update();
	//eulerianFluid.update();
	//waterDemo.update();
	testDemo.update();
}
