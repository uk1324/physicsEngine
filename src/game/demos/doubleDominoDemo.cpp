#include <game/demos/doubleDominoDemo.hpp>
#include <game/ent.hpp>

#include <imgui/imgui.h>
using namespace ImGui;

auto DoubleDominoDemo::name() -> const char* {
	return "double domino";
}

auto DoubleDominoDemo::loadSettingsGui() -> void {
	InputInt("domino count", &dominoCount);
}

auto DoubleDominoDemo::load() -> void {
	ent.body.create(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 200.0f, 100.0f } }, true });
	float height = 2.0f;
	float width = 0.5f;
	for (int i = 0; i < dominoCount; i++) {
		const auto& [_, body] = ent.body.create(Body{ Vec2{ i * (height + 0.02f), height / 2.0f }, BoxCollider{ Vec2{ width, height } } });
		body.coefficientOfFriction = 0.7f;
	}
}
