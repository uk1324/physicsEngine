#include <game/game.hpp>
#include <engine/debug.hpp>
#include <game/input.hpp>
#include <game/collision.hpp>
#include <game/distanceJoint.hpp>
#include <engine/time.hpp>
#include <engine/window.hpp>
#include <math/utils.hpp>
#include <math/mat2.hpp>
#include <engine/renderer.hpp>
#include <imgui/imgui.h>
#include <game/ent.hpp>

// spatial hashing / bucketing
// inactive flag on objects

// The exact number of collisions pairs checked in the O(n^2) broadphase is choose(n, 2).
// choose(n, 2) = n * (n - 1) / 2 = n^2 - n / 2. Taking the limit as n goes to infinity you just get n^2 / 2


#include <sstream>
#include <utils/io.hpp>
#include <json/Json.hpp>
#include <fstream>

Game::Game() {
	int height = 10;
	float boxSize = 1.0f;
	float gapSize = 0.1f;
	for (int i = 1; i < height + 1; i++) {
		for (int j = 0; j < i; j++) {
			float y = (height + 1 - i) * (boxSize + gapSize);
			float x = -i * (boxSize / 2.0f + boxSize / 8.0f) + j * (boxSize + boxSize / 4.0f);

			ent.body.create(Body{ Vec2{ x, y }, BoxColliderEditor{ Vec2{ boxSize } }, false });
		}
	}
	ent.body.create(Body{ Vec2{ 0.0f, -50.0f }, BoxColliderEditor{ Vec2{ 100.0f } }, true });

	camera.zoom = 0.125f / 2.0f;
	camera.pos = Vec2{ 0.0f, 6.0f };

	Window::maximize();
	gravity = Vec2{ 0.0f, -10.0f };
}

auto Game::detectCollisions() -> void {
	collisionSystem.update();
	collisionSystem.detectCollisions(contacts);
}

auto Game::loadLevel() -> void{
	std::stringstream buffer;
	{
		std::ifstream level("./levels/test");
		buffer << level.rdbuf();
	}
	try {
		collisionSystem.reset();
		ent.reset();

		const auto level = Json::parse(buffer.str());

		const auto& bodies = level.at("bodies").array();
		for (const auto& bodyJson : bodies) {
			const auto& [_, body] = ent.body.create(BodyOldEditor::fromJson(bodyJson));
			body.coefficientOfFriction = 0.5f;
		}

		const auto& distanceJoints = level.at("distanceJoints").array();
		for (const auto& jointJson : distanceJoints) {
			const auto joint = DistanceJointEntityEditor::fromJson(jointJson);
			auto a = ent.body.validate(joint.anchorA.body);
			auto b = ent.body.validate(joint.anchorB.body);

			if (!a.has_value() || !b.has_value()) {
				dbg("failed to load level");
				return;
			}
			ent.distanceJoint.create(DistanceJoint{ *a, *b, joint.distance });
		}

	} catch (const Json::ParsingError&) {
		dbg("failed to load level");
	}
}

auto Game::drawUi() -> void {
	using namespace ImGui;

	Begin("physics engine");
	Checkbox("update physics", &updatePhysics);
	if (!updatePhysics && Button("single step")) {
		doASingleStep = true;
	}
	Checkbox("show trajectory", &drawTrajectory);
	if (drawTrajectory) {
		SliderFloat2("initial velocity", initialVelocity.data(), -10.0f, 10.0f);
	}
	Checkbox("draw contacts", &drawContacts);
	if (drawContacts) {
		Checkbox("scale contact normals", &scaleContactNormals);
	}

	auto disableGravity = gravity == Vec2{ 0.0f };
	Checkbox("disableGravity", &disableGravity);
	if (disableGravity) {
		gravity = Vec2{ 0.0f };
	} else {
		gravity = Vec2{ 0.0f, -10.0f };
	}

	Checkbox("reuse previous contact accumulators", &reusePreviousFrameContactAccumulators);
	End();
}

auto Game::update() -> void {
	camera.aspectRatio = Window::aspectRatio();
	Vec2 dir{ 0.0f };
	if (Input::isKeyHeld(Keycode::UP)) dir.y += 1.0f;
	if (Input::isKeyHeld(Keycode::DOWN)) dir.y -= 1.0f;
	if (Input::isKeyHeld(Keycode::RIGHT)) dir.x += 1.0f;
	if (Input::isKeyHeld(Keycode::LEFT)) dir.x -= 1.0f;
	camera.pos += dir.normalized() * Time::deltaTime() / camera.zoom;
	if (Input::isKeyHeld(Keycode::J)) camera.zoom *= pow(3.0f, Time::deltaTime());
	if (Input::isKeyHeld(Keycode::K)) camera.zoom /= pow(3.0f, Time::deltaTime());
	camera.scrollOnCursorPos();

	// For positions not not lag behind the camera has to be updated first.
	const auto mousePos = camera.screenSpaceToCameraSpace(Input::cursorPos());
	Debug::drawPoint(mousePos);

	ent.update();

	if (drawTrajectory) {
		Vec2 previous = mousePos;
		for (int i = 0; i < 50; i++) {
			float x = i / 20.0f / 16.0f / camera.zoom;
			Vec2 v = mousePos + Vec2{ x * initialVelocity.x, x * x * gravity.y / 2.0f + x * initialVelocity.y };
			if ((previous - v).lengthSq() < 0.001f)
				continue;

			const auto collision = collisionSystem.raycast(previous, v);
			if (collision.has_value()) {
				Debug::drawLine(previous, previous + ((v - previous) * collision->t));
				Debug::drawPoint(previous + ((v - previous) * collision->t));
				break;
			}
			Debug::drawLine(previous, v);
			previous = v;
		}
	}
	if (Input::isKeyDown(Keycode::U)) {
		const auto& [_, body] = ent.body.create(Body{ mousePos, BoxColliderEditor{ Vec2{ 1.0f } }, false });
		body.vel = initialVelocity;
		body.angularVel = 1.5f;
		body.transform.rot = Rotation{ 0.0f };
	}

	if (Input::isKeyDown(Keycode::G)) __debugbreak();

	drawUi();
	if (Input::isKeyDown(Keycode::X)) updatePhysics = !updatePhysics;

	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		for (const auto [id, body] : ent.body) {
			if (contains(mousePos, body.transform.pos, body.transform.angle(), body.collider)) {
				selected = id;
				selectedGrabPointObjectSpace = (mousePos - body.transform.pos) * Mat2::rotate(-body.transform.angle());
			}
		}
	}
	if (Input::isMouseButtonUp(MouseButton::LEFT)) {
		selected = std::nullopt;
	}

	if (selected.has_value()) {
		auto body = ent.body.get(*selected);
		const auto offsetUprightSpace = selectedGrabPointObjectSpace * Mat2::rotate(body->transform.angle());
		const auto fromMouseToObject = mousePos - (body->transform.pos + offsetUprightSpace);
		body->force = -body->vel / (Time::deltaTime() * 5.0f) * body->mass;
		body->force += fromMouseToObject / pow(Time::deltaTime(), 2.0f) * body->mass / 10.0f;
		body->torque = det(offsetUprightSpace, body->force);
	}

	if (doASingleStep) {
		physicsStep();
		doASingleStep = false;
	} else if (updatePhysics) {
		physicsStep();
	}

	for (const auto& [_, body] : ent.body) {
		Debug::drawCollider(body.collider, body.transform.pos, body.transform.angle());
	}

	if (drawContacts) {
		for (const auto& [_, collision] : contacts) {
			for (i32 i = 0; i < collision.contactCount; i++) {
				const auto& contact = collision.contacts[i];
				const auto scale = scaleContactNormals ? contact.separation : 0.1f;
				Debug::drawRay(contact.pos, -collision.normal * scale, Vec3::RED);
			}
		}
	}

	for (const auto& [_, joint] : ent.distanceJoint) {
		auto a = ent.body.get(joint.bodyA);
		auto b = ent.body.get(joint.bodyB);
		if (!a.has_value() || !b.has_value()) {
			ASSERT_NOT_REACHED();
			continue;
		}
		Debug::drawRay(a->transform.pos, (b->transform.pos - a->transform.pos).normalized() * joint.requiredDistance);
	}

	Renderer::update(camera);
}
auto Game::physicsStep() -> void {
	for (const auto [_, body] : ent.body) {
		if (body.isStatic())
			continue;

		// It might be better to use impulses instead of forces so they are independent of the time step.
		body.vel += (body.force * body.invMass + gravity) * Time::deltaTime();
		body.force = Vec2{ 0.0f };

		body.angularVel += body.torque * body.invRotationalInertia * Time::deltaTime();
		body.angularVel *= pow(angularDamping, Time::deltaTime());
		body.torque = 0.0f;
	}

	detectCollisions();

	const auto invDeltaTime = 1.0f / Time::deltaTime();

	for (auto& [key, contact] : contacts) {
		contact.preStep(*key.a, *key.b, invDeltaTime);
	}

	for (const auto& [_, joint] : ent.distanceJoint) {
		joint.preStep(invDeltaTime);
	}

	for (int i = 0; i < 10; i++) {
		for (auto& [key, contact] : contacts) {
			contact.applyImpulse(*key.a, *key.b);
		}
		for (const auto& [_, joint] : ent.distanceJoint) {
			joint.applyImpluse();
		}
	}

	for (const auto [_, body] : ent.body) {
		if (body.isStatic())
			continue;
		body.transform.pos += body.vel * Time::deltaTime();
		body.transform.rot *= Rotation{ body.angularVel * Time::deltaTime() };
	}
}

bool Game::reusePreviousFrameContactAccumulators = true;
bool Game::positionCorrection = true;
bool Game::accumulateImpulses = true;