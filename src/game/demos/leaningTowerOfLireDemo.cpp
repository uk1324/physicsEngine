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
	// For a block to be balanced on the edge of another block it's center of mass has to touch with the block, because then the torque making the block stable is >= than the torque trying to make it fall over.
	// https://mathcs.org/analysis/reals/numser/answers/lire_tower.html
	// A harmonic series is a divergent series so in an idealized example you could do this infinitely many times, but with each new block adding less and less to the length of the tower.

	float yOffset = 0.0f;
	for (int i = 1; i <= height; i++) {
		const auto overhang = (1.0f / i) * blockWidth - moveBlocksBack;
		yOffset -= overhang;
		float y = (height - i) * blockHeight + blockHeight / 2.0f;
		ent.body.create(Body{ Vec2{ yOffset, y }, BoxCollider{ Vec2{ blockWidth, blockHeight } }, false });
	}
	ent.body.create(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 200.0f, 100.0f } }, true });
	// Couldn't get the tower to stack higher. 
	// Tried changing the order of creating entites so the impulses get solved in the opposite order, which just made things worse.
	// Tried compressing things at the start (moving the y down) so the contacts get created for all points (they weren't getting created this may be because of the bug). This kinda seemed to help when with the bug but without it it just breaks things.
	// Increasing solver iterations and substeps didn't help, but there may be some balance in which it does help.
	// The issue is that if you decrease moveBlocksBack even slightly then the tower collapses. You can keep increasing the height, but with the current value for moveBlocksBack it would just create a "C" shape, because the rate at which the harmonic series grows gets smaller than the offset.
	// Making the blocks thicker actually makes the tower easier collapse, but making the blocks really think doesn't help much.
}
