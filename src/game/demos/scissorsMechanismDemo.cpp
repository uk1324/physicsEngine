#include <game/demos/scissorsMechanismDemo.hpp>
#include <game/ent.hpp>

#include <imgui/imgui.h>
using namespace ImGui;

auto ScissorsMechanism::name() -> const char* {
	return "scissors mechanism";
}

auto ScissorsMechanism::loadSettingsGui() -> void {
	InputInt("length", &length);
}

auto ScissorsMechanism::load() -> void {
	std::vector<BodyId> bodies;
	float width = 0.5f;
	float height = 4.0f;
	auto addJoint = [this](BodyId a, BodyId b, Vec2 anchorA, Vec2 anchorB) {
		auto [jointId, _] = ent.distanceJoint.create(DistanceJoint{ a, b, 0.0f, anchorA, anchorB });
		const BodyPair bodyPair{ a, b };
		ent.revoluteJointsWithIgnoredCollisions.push_back({ jointId, bodyPair });
	};

	for (int i = 0; i < length; i++) {
		const auto pos = Vec2{ i * (height - width), 0.0f };
		const auto collider = BoxCollider{ Vec2{ height, width } };
		for (int _ = 0; _ < 2; _++) {
			const auto& [id, body] = ent.body.create(Body{ pos + Vec2{ 0.0f, 2.0f }, collider });
			bodies.push_back(id);
		}
		
		addJoint(bodies.back(), *(bodies.end() - 2), Vec2{ 0.0f }, Vec2{ 0.0f });
	}

	for (size_t i = 0; i < bodies.size() - 2; i += 2) {
		for (int j = 0; j < 2; j++) {
			const auto anchor = Vec2{ height / 2.0f - width / 2.0f, 0.0f };
			addJoint(bodies[i + j], bodies[i + j + 2], anchor, -anchor);
		}
	}

	for (const auto& a : bodies) {
		for (const auto& b : bodies) {
			if (&a != &b) {
				ent.collisionsToIgnore.insert({ a, b });
			}
		}
	}

	for (size_t i = 0; i < bodies.size() - 3; i += 2) {
		ent.collisionsToIgnore.erase({ bodies[i], bodies[i + 3] });
	}

	// Attach the endpoints to a prismatic joint?
	for (usize i = 0; i < bodies.size(); i += 2) {
		auto b0 = ent.body.get(bodies[i + 1]);
		const auto a = PI<float> / 10.0f;
		const auto b = PI<float> - a;
		b0->transform.rot = Rotation{ a };
		auto b1 = ent.body.get(bodies[i]);
		b1->transform.rot = Rotation{ b };
	}
	ent.body.create(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 200.0f, 100.0f } }, true });
}
