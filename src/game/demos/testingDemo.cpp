#include <game/demos/testingDemo.hpp>
#include <game/ent.hpp>

#include <imgui/imgui.h>
using namespace ImGui;

auto TestingDemo::name() -> const char* {
	return "testing";
}

auto TestingDemo::loadSettingsGui() -> void {

}

auto TestingDemo::load() -> void {
	static constexpr Vec2T<i64> size{ 10 };
	BodyId grid[size.x][size.y];
	const auto scale = 0.25f;
	for (int x = 0; x < size.x; x++) {
		for (int y = 0; y < size.y; y++) {
			grid[x][y] = ent.body.create(Body{ Vec2(x, y) * scale + Vec2{ 1.0f }, CircleCollider{ 0.5f * scale }, false }).id;
		}
	}

	for (int x = 0; x < size.x; x++) {
		for (int y = 0; y < size.y; y++) {
			if (x != 0) {
				ent.springJoint.create(SpringJoint{ grid[x - 1][y], grid[x][y], scale });
			}
			if (y != 0) {
				ent.springJoint.create(SpringJoint{ grid[x][y - 1], grid[x][y], scale });
			}
		}
	}
	/*const auto a = ent.body.create(Body{ Vec2{ -2.0f, 1.0f }, CircleCollider{ 0.5f }, false });
	const auto b = ent.body.create(Body{ Vec2{ 2.0f, 1.0f }, CircleCollider{ 0.5f }, false });*/
	//ent.springJoint.create(SpringJoint{ a.id, b.id, 2.0f });

	ent.body.create(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 200.0f, 100.0f } }, true });
}
