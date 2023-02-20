#include <game/demos/leaningTowerOfLireDemo.hpp>
#include <game/ent.hpp>

#include <imgui/imgui.h>
using namespace ImGui;

auto LeaningTowerOfLireDemo::name() -> const char* {
	return "leaning tower of lire";
}

auto LeaningTowerOfLireDemo::loadSettingsGui() -> void {
	Text("works well with 50 solver iterations and 50 substeps");
	InputInt("height", &height);
	InputFloat("block width", &blockWidth);
	InputFloat("block height", &blockHeight);
	InputFloat("moveBlocksBack", &moveBlocksBack);
}

auto LeaningTowerOfLireDemo::load() -> void {
	std::vector<Body> body;
	float h = 0.0f;
	for (int i = 1; i <= height; i++) {
		const auto overhang = (1.0f / i) * blockWidth - moveBlocksBack;
		h -= overhang;
		float y = (height - i) * blockHeight + blockHeight / 2.0f ;
		ent.body.create(Body{ Vec2{ h, y - 0.01f }, BoxCollider{ Vec2{ blockWidth, blockHeight } }, false });
	}
	ent.body.create(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 200.0f, 100.0f } }, true });
}
