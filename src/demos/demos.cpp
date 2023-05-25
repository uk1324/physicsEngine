#include <demos/demos.hpp>
#include <imgui/imgui.h>

auto Demos::update() -> void {
	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_NoDockingInCentralNode | ImGuiDockNodeFlags_PassthruCentralNode);
	pixelPhysics.update();
	//triangulationDemo.update();
	//fourierTransformDemo.update();
	//marchingSquaresDemo.update();
	//eulerianFluid.update();
	//maxwell.update();
	//waterDemo.update();
	//testDemo.update();
	//integrationDemo.update();
	//polygonHullDemo.update();'
	//imageToSdfDemo.update();
	//gravityInDifferentMetricsDemo.update();
}
