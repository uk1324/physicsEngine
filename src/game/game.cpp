#include <game/game.hpp>
#include <game/debug.hpp>
#include <game/input.hpp>
#include <game/collision/collision.hpp>
#include <game/distanceJoint.hpp>
#include <engine/time.hpp>
#include <engine/window.hpp>
#include <math/utils.hpp>
#include <math/mat2.hpp>
#include <imgui/imgui.h>

// TODO: Understand gauss–Seidel method
// spatial hashing / bucketing
// inactive flag on objects

// The exact number of collisions checked in the O(n^2) broadphase is choose(n, 2).


CollisionMap contacts;
std::unordered_map<BodyPair, DistanceJoint, BodyPairHasher> joints;
std::vector<Body> bodies;

#include <sstream>
#include <utils/io.hpp>
#include <json/Json.hpp>


Game::Game(Gfx& gfx)
	: renderer{ gfx } {

	int height = 14;
	float boxSize = 1.0f;
	float gapSize = 0.1f;
	for (int i = 1; i < height + 1; i++) {
		for (int j = 0; j < i; j++) {
			float y = (height + 1 - i) * (boxSize + gapSize);
			float x = -i * (boxSize / 2.0f + boxSize / 8.0f) + j * (boxSize + boxSize / 4.0f);

			bodies.push_back(Body{ Vec2{ x, y }, BoxCollider{ Vec2{ boxSize } }, false });
		}
	}
	/*bodies.push_back(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 100.0f } }, true });*/
	bodies.push_back(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 100.0f } }, true });
	/*bodies.push_back(Body{ Vec2{ -2.0, 7.0 }, BoxCollider{ Vec2{ 1.0f, 1.0f } }, true });
	bodies.push_back(Body{ Vec2{ -1.0, 4.0 }, BoxCollider{ Vec2{ 1.0f, 1.0f } }, false });
	joints[BodyPair{ &bodies[bodies.size() - 1], &bodies[bodies.size() - 2] }] = DistanceJoint{ .requiredDistance = 4.0f };*/
	//bodies.push_back(Body{ Vec2{ 1.0, 1.0 }, BoxCollider{ Vec2{ 1.0f, 1.0f } }, false });
	//bodies.push_back(Body{ Vec2{ 1.0, 1.0 }, BoxCollider{ Vec2{ 1.0f, 1.0f } }, false });
	//bodies.push_back(Body{ Vec2{ 1.0, 7.0 }, BoxCollider{ Vec2{ 1.0f, 1.0f } }, false });
	//bodies.push_back(Body{ Vec2{ 1.0, 7.0 }, BoxCollider{ Vec2{ 1.0f, 1.0f } }, false });
	//bodies.push_back(Body{ Vec2{ 3.0, 7.0 }, BoxCollider{ Vec2{ 1.0f, 1.0f } }, false });
	//joints[BodyPair{ &bodies[bodies.size() - 2], &bodies[bodies.size() - 3] }] = DistanceJoint{ .requiredDistance = 4.0f };
	//joints[BodyPair{ &bodies[bodies.size() - 3], &bodies[bodies.size() - 4] }] = DistanceJoint{ .requiredDistance = 4.0f };
	//joints[BodyPair{ &bodies[bodies.size() - 4], &bodies[bodies.size() - 5] }] = DistanceJoint{ .requiredDistance = 4.0f };
	//joints[BodyPair{ &bodies[bodies.size() - 5], &bodies[bodies.size() - 6] }] = DistanceJoint{ .requiredDistance = 4.0f };
	//joints[BodyPair{ &bodies[bodies.size() - 6], &bodies[bodies.size() - 7] }] = DistanceJoint{ .requiredDistance = 4.0f };
	camera.zoom = 0.125f / 2.0f;
	camera.pos = Vec2{ 0.0f, 6.0f };
	
	/*bodies.push_back(Body{ Vec2{ 0.0f, 10.0f }, CircleCollider{ 0.5f }, false });
	bodies.push_back(Body{ Vec2{ 0.0f, 7.0f }, CircleCollider{ 0.5f }, false });*/
	//bodies.push_back(Body{ Vec2{ 100.0f, 100.0f }, BoxCollider{ Vec2{ 1.0f, 0.5f } }, false });

	static std::vector<Body*> vAdd;
	for (auto& body : bodies) {
		vAdd.push_back(&body);
	}
	static const std::vector<Body*> vDelete;
	collisionSystem.update(vAdd, vDelete);

	Window::maximize();
	/*followedPos = &bodies[0].pos;
	controlledValue = &bodies[0].vel;*/
	gravity = Vec2{ 0.0f, -10.0f };

	std::stringstream s;
	const auto json = bodies[0].toJson();
	Json::prettyPrint(s, json);
	put("%s", s.str().data());
	const auto x = Body::fromJson(json);
}

auto doCollision() -> void {
	auto start{ bodies.begin() };
	for (auto& a : bodies) {
		start++;
		for (auto it = start; it != bodies.end(); it++) {
			auto& b{ *it };

			if (a.isStatic() && b.isStatic())
				continue;

			BodyPair key{ &a, &b };

			// TODO: Can be made const.
			if (auto collision = collide(a.pos, a.orientation, a.collider, b.pos, b.orientation, b.collider); collision.has_value()) {
				// TODO: Move this into some function or constructor probably when making a better collision system.
				collision->coefficientOfFriction = sqrt(a.coefficientOfFriction * b.coefficientOfFriction);
				if (const auto& oldContact = contacts.find(key); oldContact == contacts.end()) {
					contacts[key] = *collision;
				} else {
					oldContact->second.update(collision->contacts, collision->contactCount);
				}
			} else {
				contacts.erase(key);
			}
		}
	}
}

static auto drawBox(Vec2 size, Vec2 pos, float orientation) -> void {
	const auto rotate = Mat2::rotate(orientation);
	// @Performance: Could just use the basis from the rotate matrix. Or even better precompute the matrix because it is used in a lot of places.
	const auto edgeX = Vec2{ size.x, 0.0f } *rotate;
	const auto edgeY = Vec2{ 0.0f, size.y } *rotate;
	const auto vertex1 = (size / 2.0f) * rotate + pos;
	const auto vertex2 = vertex1 - edgeX;
	const auto vertex3 = vertex2 - edgeY;
	const auto vertex4 = vertex3 + edgeX;
	const auto color = Vec3{ 1.0f };
	Debug::drawLine(vertex1, vertex2, color);
	Debug::drawLine(vertex2, vertex3, color);
	Debug::drawLine(vertex3, vertex4, color);
	Debug::drawLine(vertex4, vertex1, color);
};

auto Game::detectCollisions() -> void {
	static const std::vector<Body*> v;
	collisionSystem.update(v, v);
	collisionSystem.detectCollisions(contacts);
}

auto Game::drawUi() -> void {
	using namespace ImGui;

	Begin("physics engine");
	Checkbox("update physics", &updatePhysics);
	if (followedPos == nullptr) {
		cameraFollow = false;
	} else {
		// Could also use ImGui::BeginDisabled().
		Checkbox("camera follow", &cameraFollow);
	}
	Checkbox("show trajectory", &drawTrajectory);
	if (drawTrajectory) {
		SliderFloat2("initial velocity", initialVelocity.data(), -10.0f, 10.0f);
	}
	Checkbox("draw contacts", &drawContacts);
	End();
}

auto Game::update(Gfx& gfx) -> void {
	camera.aspectRatio = Window::size().x / Window::size().y;
	if (cameraFollow && followedPos != nullptr) {
		camera.interpolateTo(*followedPos, 2.0f * Time::deltaTime());
	} else {
		Vec2 dir{ 0.0f };
		if (Input::isKeyHeld(Keycode::UP)) dir.y += 1.0f;
		if (Input::isKeyHeld(Keycode::DOWN)) dir.y -= 1.0f;
		if (Input::isKeyHeld(Keycode::RIGHT)) dir.x += 1.0f;
		if (Input::isKeyHeld(Keycode::LEFT)) dir.x -= 1.0f;
		camera.pos += dir.normalized() * Time::deltaTime() / camera.zoom;
	}

	if (Input::isKeyHeld(Keycode::J)) camera.zoom *= pow(3.0f, Time::deltaTime());
	if (Input::isKeyHeld(Keycode::K)) camera.zoom /= pow(3.0f, Time::deltaTime());

	// For positions not not lag behind the camera has to be updated first.

	const auto mousePos = camera.screenSpaceToCameraSpace(Input::cursorPos());
	Debug::drawPoint(mousePos);
	
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
		bodies.back().pos = mousePos;
		bodies.back().vel = initialVelocity;
		bodies.back().angularVel = 1.5f;
		bodies.back().orientation = 0.0f;
	}

	if (Input::isKeyDown(Keycode::G)) __debugbreak();

	drawUi();
	if (Input::isKeyDown(Keycode::X)) updatePhysics = !updatePhysics;

	if (controlledValue != nullptr) {
		Vec2 dir{ 0.0f };
		if (Input::isKeyHeld(Keycode::W)) dir.y += 1.0f;
		if (Input::isKeyHeld(Keycode::S)) dir.y -= 1.0f;
		if (Input::isKeyHeld(Keycode::D)) dir.x += 1.0f;
		if (Input::isKeyHeld(Keycode::A)) dir.x -= 1.0f;
		
		(*controlledValue) += dir.normalized() * 0.5f * Time::deltaTime() * 10.0f;
	}

	static Body* selected;
	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		for (auto& body : bodies) {
			if (contains(mousePos, body.pos, body.orientation, body.collider))
				selected = &body;
		}
	}
	if (Input::isMouseButtonUp(MouseButton::LEFT)) {
		selected = nullptr;
	}

	if (selected != nullptr) {
		const auto fromMouseToObject = mousePos - selected->pos;
		selected->force = fromMouseToObject * 400.0f * fromMouseToObject.length() * 3.0f;
	}

	if (updatePhysics) {
		for (auto& body : bodies) {
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
			contact.preStep(key.a, key.b, invDeltaTime);
		}

		for (auto& [key, joint] : joints) {
			joint.preStep(*key.a, *key.b, invDeltaTime);
		}

		for (int i = 0; i < 10; i++) {
			for (auto& [key, contact] : contacts) {
				contact.applyImpulse(*key.a, *key.b);
			}
			for (auto& [key, joint] : joints) {
				joint.applyImpluse(*key.a, *key.b);
			}
		}

		for (auto& body : bodies) {
			if (body.isStatic())
				continue;
			body.pos += body.vel * Time::deltaTime();
			body.orientation += body.angularVel * Time::deltaTime();
		}
	}

	for (const auto& body : bodies) {
		if (const auto box = std::get_if<BoxCollider>(&body.collider); box != nullptr) {
			drawBox(box->size, body.pos, body.orientation);
		} else if (const auto circle = std::get_if<CircleCollider>(&body.collider); circle != nullptr) {
			Debug::drawEmptyCircle(body.pos, circle->radius, body.orientation);
		}
	}

	if (drawContacts) {
		for (const auto& [_, collision] : contacts) {
			for (i32 i = 0; i < collision.contactCount; i++) {
				const auto& contact = collision.contacts[i];
				Debug::drawRay(contact.position, contact.normal * 0.1f, Vec3::RED);
			}
		}
	}

	for (const auto& [bodyPair, joint] : joints) {
		Debug::drawRay(bodyPair.a->pos, (bodyPair.b->pos - bodyPair.a->pos).normalized() * joint.requiredDistance);
	}

	renderer.update(gfx, camera);
}	

bool Game::warmStarting = true;
bool Game::positionCorrection = true;
bool Game::accumulateImpulses = true;