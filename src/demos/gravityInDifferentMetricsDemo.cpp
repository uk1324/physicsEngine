#include <demos/gravityInDifferentMetricsDemo.hpp>
#include <engine/input.hpp>
#include <engine/time.hpp>
#include <utils/dbg.hpp>

using namespace ImGui;

GravityInDifferentMetricsDemo::GravityInDifferentMetricsDemo() {
	camera.zoom /= 8.0f;
}

auto GravityInDifferentMetricsDemo::update() -> void {
	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		if (!selectedPos.has_value()) {
			selectedPos = camera.cursorPos();
		} else {
			bodies.push_back(Body{ camera.cursorPos(), selectedRadius, selectedMass, camera.cursorPos() - *selectedPos });
			bodies.back().trial.push_back(bodies.back().pos);
			selectedPos = std::nullopt;
		}
	}
	camera.scrollOnCursorPos();

	Begin("gravity");
	InputFloat("radius", &selectedRadius);
	InputFloat("mass", &selectedMass);
	//InputFloat2("initial velocity", initialVelocity.data());
	End();

	Debug::drawRay(camera.cursorPos(), initialVelocity);
	for (const auto& body : bodies) {
		Debug::drawHollowCircle(body.pos, body.radius);
		Debug::drawText(body.pos, body.mass);
		if (body.trial.size() >= 1) {
			for (usize i = 0; i < body.trial.size() - 1; i++) {
				Debug::drawLine(body.trial[i], body.trial[i + 1]);
			}
			Debug::drawLine(body.trial.back(), body.pos);
		}
	}

	int substeps = 5;
	const auto dt = Time::deltaTime() / substeps;
	for (int i = 0; i < substeps; i++) {
		auto start{ bodies.begin() };
		for (auto& a : bodies) {
			start++;
			for (auto it = start; it != bodies.end(); it++) {
				auto& b{ *it };

				const auto distance = a.pos - b.pos;
				const auto force = distance * (gravitationalConstant * a.mass * b.mass) / dot(distance, distance);
				a.vel -= force * dt / a.mass;
				b.vel += force * dt / b.mass;
			}
		}

		for (auto& body : bodies) {
			body.pos += body.vel * dt;
		}
	}

	for (auto& body : bodies) {
		const auto lastPos = body.trial.back();
		if (distance(body.pos, lastPos) > 0.1f) {
			body.trial.push_back(body.pos);
		}
	}

	Renderer::update(camera, powf(2.0, round(log2f(1.0f / camera.zoom / 25.0f))));
}
