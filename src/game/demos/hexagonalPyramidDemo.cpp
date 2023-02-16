#include <game/demos/hexagonalPyramidDemo.hpp>
#include <game/ent.hpp>

#include <imgui/imgui.h>
using namespace ImGui;

auto HexagonalPyramid::name() -> const char* {
	return "hexagonal pyramid";
}

auto HexagonalPyramid::loadSettingsGui() -> void {
	InputInt("pyramid height", &pyramidHeight);
}

auto HexagonalPyramid::load() -> void {
	ent.body.create(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 200.0f, 100.0f } }, true });
	float gapSize = 0.1f;
	float radius = 1.0f;
	for (int i = 1; i < pyramidHeight + 1; i++) {
		std::vector<BodyId> bodies;
		for (int j = 0; j < i; j++) {
			float y = (pyramidHeight + 1 - i) * (radius + gapSize);
			float x = -i * (radius / 2.0f + radius / 8.0f) + j * (radius + radius / 4.0f);

			float radius = 0.7f;
			if (i == pyramidHeight) {
				auto gon = ConvexPolygon::regular(6, radius);
				gon.verts.pop_back();
				gon.calculateNormals();
				const auto& [id, b] = ent.body.create(Body{ Vec2{ x, y }, gon, false });
				bodies.push_back(id);
				b.transform.rot *= Rotation{ -TAU<float> / 6.0f };
			} else {
				const auto& [id, _] = ent.body.create(Body{ Vec2{ x, y }, ConvexPolygon::regular(6, radius), false });
				bodies.push_back(id);
			}
		}

		if (i == pyramidHeight) {
			for (size_t j = 0; j < bodies.size() - 1; j++) {
				ent.distanceJoint.create(DistanceJoint{ bodies[j], bodies[j + 1], radius + radius / 4.0f, Vec2{ 0.0f }, Vec2{ 0.0f } });
			}
		}
	}
}

