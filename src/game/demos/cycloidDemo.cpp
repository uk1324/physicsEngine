#include <game/demos/cycloidDemo.hpp>
#include <game/ent.hpp>
#include <engine/debug.hpp>

#include <imgui/imgui.h>
using namespace ImGui;

auto CycloidDemo::name() -> const char* {
	return "cycloid";
}

auto CycloidDemo::loadSettingsGui() -> void {
	InputFloat("cycloid radius", &cycloidRadius);
}

// https://en.wikipedia.org/wiki/Cycloid
// https://en.wikipedia.org/wiki/Brachistochrone_curve
// https://en.wikipedia.org/wiki/Tautochrone_curve
// The original problem was posed as a bead on a frictionless wire so this is using zero friction to make it more correct.
auto CycloidDemo::load() -> void {
	std::vector<BodyId> spawnedCircles;
	auto makeLine = [&spawnedCircles](Vec2 p0, Vec2 p1, bool spawnCircle) -> void {
		const auto diff = p1 - p0;
		Vec2 size{ diff.length(), 1.0f };
		const Rotation rotation{ (diff).angle() };
		auto b = ent.body.create(Body{ (p0 + p1) / 2.0f - (Vec2{ size.y / 2.0f, 0.0f } * rotation * Rotation{ PI<float> / 2.0f }), BoxCollider{ size }, true });
		b->transform.rot = rotation;
		b->coefficientOfFriction = 0.0f;
		if (spawnCircle) {
			const auto [id, body] = ent.body.create(Body{ (p0 + p1) / 2.0f + (Vec2{ 1.0f, 0.0f } *rotation * Rotation{ PI<float> / 2.0f }), CircleCollider{ 1.0f }, false });
			spawnedCircles.push_back(id);
			body.coefficientOfFriction = 0.0f;
		}
	};
	Vec2 previous{ 0.0f, cycloidRadius };
	int samples = 100;
	bool spawnCircle = false;
	for (int i = 1; i <= samples; i++) {
		if (!spawnCircle)
			spawnCircle = i % 10 == 0 && i <= samples * 0.4f;
		const auto t = i / static_cast<float>(samples);
		Vec2 pos = posOnCycloid(t);;
		const auto difference = pos - previous;
		if (difference.length() < 2.0f && i != samples) {
			continue;
		}
		Vec2 size{ 0.4f, difference.length() };
		makeLine(previous, pos, spawnCircle);
		spawnCircle = false;
		previous = pos;
	}

	// TODO: Combinations iterator class? Currently this loop creates duplicate pairs.
	for (const auto& a : spawnedCircles) {
		for (const auto& b : spawnedCircles) {
			ent.collisionsToIgnore.insert({ a, b });
		}
	}
}

auto CycloidDemo::settingsGui() -> void {
	Checkbox("show cricle", &showCircle);
	if (showCircle) {
		SliderFloat("circle t", &circleT, 0.0f, 1.0f);
	}
}

auto CycloidDemo::update(const DemoData&) -> void {
	const auto t = circleT;
	const auto circleCenter = Vec2{ t * cycloidRadius * TAU<float>, 0.0f };
	if (showCircle) {
		Debug::drawHollowCircle(circleCenter, cycloidRadius);
		Debug::drawLine(circleCenter, posOnCycloid(t));
	}
}

auto CycloidDemo::posOnCycloid(float t) const -> Vec2 {
	// adding PI / 2 to make [0, r] the starting point instead of [r, 0].
	const auto pointOnRotatingCircle = Vec2::oriented(t * TAU<float> + PI<float> / 2.0f) * cycloidRadius;
	const auto xMovementFromRolling = Vec2{ t * (cycloidRadius * TAU<float>), 0.0f };
	return pointOnRotatingCircle + xMovementFromRolling;
}
