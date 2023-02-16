#include <game/demos/pyramidDemo.hpp>
#include <game/ent.hpp>

#include <imgui/imgui.h>
using namespace ImGui;

auto PyramidDemo::name() -> const char* {
	return "pyramid";
}

auto PyramidDemo::loadSettingsGui() -> void {
	InputInt("pyramid height", &pyramidHeight);
	InputFloat("boxSize", &boxSize);
}

auto PyramidDemo::load() -> void {
	ent.body.create(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 200.0f, 100.0f } }, true });
	float gapSize = 0.1f;
	for (int i = 1; i < pyramidHeight + 1; i++) {
		for (int j = 0; j < i; j++) {
			float y = (pyramidHeight + 1 - i) * (boxSize + gapSize);
			float x = -i * (boxSize / 2.0f + boxSize / 8.0f) + j * (boxSize + boxSize / 4.0f);

			ent.body.create(Body{ Vec2{ x, y }, BoxCollider{ Vec2{ boxSize } }, false });
		}
	}
}
