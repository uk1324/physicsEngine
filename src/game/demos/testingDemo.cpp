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
	//static constexpr Vec2T<i64> size{ 10 };
	//BodyId grid[size.x][size.y];
	//const auto scale = 0.25f;
	//for (int x = 0; x < size.x; x++) {
	//	for (int y = 0; y < size.y; y++) {
	//		grid[x][y] = ent.body.create(Body{ Vec2(x, y) * scale + Vec2{ 1.0f }, CircleCollider{ 0.5f * scale }, false }).id;
	//	}
	//}

	//for (int x = 0; x < size.x; x++) {
	//	for (int y = 0; y < size.y; y++) {
	//		if (x != 0) {
	//			ent.springJoint.create(SpringJoint{ grid[x - 1][y], grid[x][y], scale });
	//		}
	//		if (y != 0) {
	//			ent.springJoint.create(SpringJoint{ grid[x][y - 1], grid[x][y], scale });
	//		}
	//	}
	//}
	auto circle = [](Vec2 pos, float size, bool offset = false) {
		int steps = 20;
		std::vector<BodyId> outer;
		for (int i = 0; i < steps; i++) {
			auto angle = i * TAU<float> / steps;
			if (offset) {
				angle += TAU<float> / steps / 2.0f;
			}
			auto body = ent.body.create(Body{ Vec2::oriented(angle) * size + pos, CircleCollider{ 0.5f * 0.25f }, false }).id;
			outer.push_back(body);
			
		}
		return outer;
	};
	auto join = [](BodyId a, BodyId b) {
		const auto d = distance(ent.body.get(a)->transform.pos, ent.body.get(b)->transform.pos);
		ent.springJoint.create(SpringJoint{ a, b, d });
	};
	auto connect = [&](const std::vector<BodyId> bodies) {
		auto previous = bodies.back();
		for (auto body : bodies) {
			join(body, previous);
			previous = body;
		}
	};


	auto makeBall = [&](Vec2 pos, float size) {
		auto outer = circle(pos, 1.0f * size);
		connect(outer);

		auto inner = circle(pos, 0.7f * size);
		connect(inner);

		auto get = [](const std::vector<BodyId>& body, int i) {
			if (i >= body.size()) {
				i = 0;
			}
			return body[i];
		};

		for (int i = 0; i < inner.size(); i++) {
			join(get(outer, i), get(inner, i));
			join(get(outer, i), get(inner, i + 1));
			join(get(outer, i + 1), get(inner, i));
		}
	};
	//makeBall(Vec2{ 0.0f, 5.0f }, 1.0f);
	for (int i = 0; i < 25; i++) {
		makeBall(Vec2{ 0.0f, 5.0f + i * 5.0f }, 1.0f);
	}
	/*makeBall(Vec2{ 5.0f, 5.0f }, 1.0f);
	makeBall(Vec2{ 10.0f, 5.0f }, 1.0f);
	makeBall(Vec2{ 15.0f, 5.0f }, 1.0f);
	makeBall(Vec2{ 20.0f, 5.0f }, 1.0f);
	makeBall(Vec2{ 25.0f, 5.0f }, 1.0f);*/


	/*const auto a = ent.body.create(Body{ Vec2{ -2.0f, 1.0f }, CircleCollider{ 0.5f }, false });
	const auto b = ent.body.create(Body{ Vec2{ 2.0f, 1.0f }, CircleCollider{ 0.5f }, false });*/
	//ent.springJoint.create(SpringJoint{ a.id, b.id, 2.0f });

	ent.body.create(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 200.0f, 100.0f } }, true });
}
