#include <demos/testDemo.hpp>
#include <game/collision.hpp>
#include <engine/debug.hpp>
#include <engine/input.hpp>
#include <math/mat2.hpp>
#include <engine/renderer.hpp>
#include <math/transform.hpp>
#include <engine/frameAllocator.hpp>
#include <imgui/imgui.h>
#include <string>

#include <math/utils.hpp>

auto drawPolygon(const ConvexPolygon& polygon, const Transform& transform) {
	const auto& [verts, normals] = polygon;
	for (usize i = 0; i < verts.size() - 1; i++) {
		Debug::drawLine(verts[i] * transform, verts[i + 1] * transform);
	}
	Debug::drawLine(verts.back() * transform, verts[0] * transform);

	for (usize i = 0; i < verts.size(); i++) {
		Debug::drawText(verts[i] * transform, frameAllocator.format("v%zu", i).data(), Vec3::WHITE, 0.03f);
	}

	for (usize i = 0; i < verts.size() - 1; i++) {
		auto midPoint = (verts[i] * transform + verts[i + 1] * transform) / 2.0f;
		Debug::drawText(midPoint, frameAllocator.format("f%zu", i).data(), Vec3::WHITE, 0.03f);
	}
	auto midPoint = (verts.back() * transform + verts[0] * transform) / 2.0f;
	Debug::drawText(midPoint, frameAllocator.format("f%zu", verts.size() - 1).data(), Vec3::WHITE, 0.03f);
}

auto TestDemo::update() -> void {
	Camera camera;

	static float aOrientation = 0.0f;
	if (Input::isKeyHeld(Keycode::A)) {
		aOrientation += 0.01f;
	} else if (Input::isKeyHeld(Keycode::D)) {
		aOrientation -= 0.01f;
	}

	using namespace ImGui;

	Begin("collision");
	TextWrapped("The shape controlled by the mouse is A the other shape is B. The green text above the contact points is the id. The first part is the feature on A the second part is the feature on B");
	SliderInt("a vertex count", &aVertexCount, 3, 10);
	SliderFloat("a size", &aSize, 0.1f, 0.6f);
	SliderInt("b vertex count", &bVertexCount, 3, 10);
	SliderFloat("b size", &bSize, 0.1f, 0.6f);
	End();
	
	Transform aTransform{ camera.cursorPos(), aOrientation };
	auto a = ConvexPolygon::regular(aVertexCount, aSize);

	Transform bTransform{ Vec2{ 0.3f, 0.2f }, 0.0f };
	auto b = ConvexPolygon::regular(bVertexCount, bSize);

	drawPolygon(a, aTransform);
	drawPolygon(b, bTransform);
	auto manifold = collide(aTransform, a, bTransform, b);
	if (manifold.has_value()) {
		for (i32 i = 0; i < manifold->contactCount; i++) {
			const auto& point = manifold->contacts[i];

			auto id = (point.id.featureOnA == ContactPointFeature::FACE ? "f" : "v") + std::to_string(point.id.featureOnAIndex)
				+ ((point.id.featureOnB == ContactPointFeature::FACE) ? "f" : "v") + std::to_string(point.id.featureOnBIndex);
			Debug::drawText(point.pos + Vec2{ 0.0f, 0.05f }, id.data(), Vec3::GREEN, 0.03f);
			Debug::drawPoint(point.pos, Vec3::RED);
			Debug::drawRay(point.pos, manifold->normal * point.separation, Vec3::GREEN);
			//Debug::drawRay(point.pos, manifold->normal, Vec3::GREEN);
			//Debug::drawPoint(point.pos + manifold->normal * point.separation, Vec3::RED);

		}
	}

	Renderer::update(camera);
}
