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
#include <fstream>


Game::Game() {
	//int height = 14;
	//float boxSize = 1.0f;
	//float gapSize = 0.1f;
	//for (int i = 1; i < height + 1; i++) {
	//	for (int j = 0; j < i; j++) {
	//		float y = (height + 1 - i) * (boxSize + gapSize);
	//		float x = -i * (boxSize / 2.0f + boxSize / 8.0f) + j * (boxSize + boxSize / 4.0f);

	//		bodies.push_back(Body{ Vec2{ x, y }, BoxColliderEditor{ Vec2{ boxSize } }, false });
	//	}
	//}
	//bodies.push_back(Body{ Vec2{ 100.0f, 100.0f }, BoxColliderEditor{ Vec2{ 5.0f, 2.5f } }, false });


	///*bodies.push_back(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 100.0f } }, true });*/
	//bodies.push_back(Body{ Vec2{ 0.0f, -50.0f }, BoxColliderEditor{ Vec2{ 100.0f } }, true });

	//int count = 4;
	//for (int i = 0; i < count; i++) {
	//	bodies.push_back(Body{ Vec2{ static_cast<float>(i), 6.0f }, CircleColliderEditor{ 0.5f }, false });
	//}

	//for (int i = 1; i < count; i++) {
	//	joints[BodyPair{ &bodies[bodies.size() - i], &bodies[bodies.size() - i - 1] }] = DistanceJoint{ .requiredDistance = 2.0f };
	//	//bodies.push_back(Body{ Vec2{ -1.0, 4.0 }, BoxColliderEditor{ Vec2{ 1.0f, 1.0f } }, false });
	//}
	loadLevel();

	camera.zoom = 0.125f / 2.0f;
	camera.pos = Vec2{ 0.0f, 6.0f };

	static std::vector<Body*> vAdd;
	for (auto& body : bodies) {
		vAdd.push_back(&body);
	}
	static const std::vector<Body*> vDelete;
	collisionSystem.update(vAdd, vDelete);

	Window::maximize();
	gravity = Vec2{ 0.0f, -10.0f };
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

auto Game::detectCollisions() -> void {
	static const std::vector<Body*> v;
	collisionSystem.update(v, v);
	collisionSystem.detectCollisions(contacts);
}

auto Game::loadLevel() -> void{
	std::stringstream buffer;
	{
		std::ifstream level("./levels/test");
		buffer << level.rdbuf();
	}
	try {
		const auto level = Json::parse(buffer.str());
		
		const auto& bodies = level.at("bodies").array();
		std::vector<Body*> vAdd;
		std::vector<Body*> vDelete;

		for (auto& body : ::bodies) {
			vDelete.push_back(&body);
		}
		collisionSystem.update(vAdd, vDelete);
		::bodies.clear();
		collisionSystem.reset();

		vDelete.clear();

		for (const auto& body : bodies) {
			::bodies.push_back(BodyEditor::fromJson(body));
		}

		for (auto& body : ::bodies) {
			vAdd.push_back(&body);
		}

		collisionSystem.update(vAdd, vDelete);

	} catch (const Json::ParsingError&) {
		dbg("failed to load level");
	}
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

auto Game::update(Gfx& gfx, Renderer& renderer) -> void {
	camera.aspectRatio = Window::aspectRatio();
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
	
	if (Input::isKeyDown(Keycode::T)) {
		std::vector<Body*> toAdd;
		std::vector<Body*> toRemove;
		collisionSystem.update(toAdd, toRemove);
		contacts.clear();
	}

	static auto level = Json::Value::emptyObject();
	if (Input::isKeyDown(Keycode::V)) {
		level = Json::Value::emptyObject();
		level["bodies"] = Json::Value::emptyArray();
		auto& bodyList = level["bodies"].array();
		for (const auto& body : ::bodies) {
			bodyList.push_back(body.toJson());
		}
		std::ofstream file("test.json");
		//Json::prettyPrint(file, level);
	}
	if (Input::isKeyDown(Keycode::B)) {
		try {
			contacts.clear();
			auto& bodies = level.at("bodies").array();
			std::vector<Body*> toAdd;
			std::vector<Body*> toRemove;
			for (auto& body : ::bodies) {
				toRemove.push_back(&body);
			}
			collisionSystem.update(toAdd, toRemove);
			::bodies.clear();
			for (const auto& body : bodies) {
				::bodies.push_back(Body{ BodyEditor::fromJson(body) });
				::bodies.back().updateInvMassAndInertia();
			}
			for (auto& body : ::bodies) {
				toAdd.push_back(&body);
			}
			toRemove.clear();
			collisionSystem.update(toAdd, toRemove);
		} catch (const Json::ParsingError&) {

		} catch (const Json::Value::OutOfRangeAccess&) {

		} catch (const std::out_of_range&) {

		}
	}

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
		Debug::drawCollider(body.collider, body.pos, body.orientation);
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

	renderer.update(gfx, camera, Window::size(), false);
}	

bool Game::warmStarting = true;
bool Game::positionCorrection = true;
bool Game::accumulateImpulses = true;