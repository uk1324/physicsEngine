#include <game/game2.hpp>
#include <game/body.hpp>
#include <game/debug.hpp>
#include <game/input.hpp>
#include <game/contact.hpp>
#include <engine/time.hpp>
#include <engine/window.hpp>
#include <math/utils.hpp>
#include <math/mat2.hpp>

// TODO: Make camera struct that has functions like mousePosToScreenPos and it is passed to the renderer.

std::vector<Body> bodies;
std::unordered_map<ContactKey, Contact, ContactKeyHasher> contacts;

auto pointInBox(Vec2 point, Vec2 pos, Vec2 size, float orientation) -> bool {
	const auto basis = Mat2::rotate(orientation);
	const auto halfSize = size / 2.0f;
	const auto p = point - pos;

	const auto 
		alongY = dot(p, basis.y()),
		alongX = dot(p, basis.x());
	if (alongX > -halfSize.x && alongX < halfSize.x
		&& alongY > -halfSize.y && alongY < halfSize.y) {
		return true;
	}
	return false;
}

Game2::Game2(Gfx& gfx)
	: renderer{ gfx } {

	int height = 5;
	float boxSize = 1.0f;
	float gapSize = 0.1f;
	for (int i = 1; i < height + 1; i++) {
		for (int j = 0; j < i; j++) {
			float y = (height + 1 - i) * (boxSize + gapSize);
			float x = -i * (boxSize / 2.0f + boxSize / 8.0f) + j * (boxSize + boxSize / 4.0f);

			bodies.push_back(Body{ Vec2{ x, y }, Vec2{ boxSize }, false });
		}
	}
	bodies.push_back(Body{ Vec2{ 0.0f, -50.0f }, Vec2{ 100.0f }, true });
	cameraZoom = 0.125f;

	/*cameraZoom = 0.25f;
	bodies.push_back(Body{ Vec2{ 0.0f, 2.8f }, Vec2{ 1.0f }, false });
	bodies.push_back(Body{ Vec2{ 0.0f, 1.8f }, Vec2{ 1.0f }, false });
	bodies.push_back(Body{ Vec2{ 0.0f, -50.0f }, Vec2{ 100.0f }, true });*/
	//bodies.push_back(Body{ Vec2{ 1.1f, 1.8f }, Vec2{ 1.0f }, false });
	///*bodies.push_back(Body{ Vec2{ 0.0f, 0.4f }, Vec2{ 1.0f }, false });
	//bodies.push_back(Body{ Vec2{ 0.4f, 0.4f }, Vec2{ 0.2f, 0.2f }, false });*/
	
	Window::maximize();
	followedPos = &bodies[0].pos;
	controlledValue = &bodies[0].vel;
	/*controlledValue = &bodies[bodies.size() / 2.0f].vel;
	followedPos = &bodies[bodies.size() / 2.0f].pos;*/
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

			Contact contact{ &a, &b };
			ContactKey key{ &a, &b };

			if (contact.numContacts > 0) {
				if (const auto& oldContact = contacts.find(key); oldContact == contacts.end()) {
					contacts[key] = contact;
				} else {
					oldContact->second.Update(contact.contacts, contact.numContacts);
				}
			} else {
				contacts.erase(key);
			}
		}
	}
}

auto Game2::update(Gfx& gfx) -> void {
	static float elapsed = 0.0f;
	elapsed += Time::deltaTime();

	const auto mousePos = renderer.mousePosToScreenPos(Input::cursorPos());
	//bodies[0].pos = mousePos;
	//bodies[0].orientation = elapsed;
	//bodies[0].orientation = 0.3f;

	if (controlledValue != nullptr)
	{
		Vec2 dir{ 0.0f };
		if (Input::isKeyHeld(Keycode::W)) dir.y += 1.0f;
		if (Input::isKeyHeld(Keycode::S)) dir.y -= 1.0f;
		if (Input::isKeyHeld(Keycode::D)) dir.x += 1.0f;
		if (Input::isKeyHeld(Keycode::A)) dir.x -= 1.0f;
		
		(*controlledValue) += dir.normalized() * 0.5f * Time::deltaTime() * 10.0f;
	}

	if (Input::isKeyDown(Keycode::G)) {
		__debugbreak();
	}

	if (Input::isKeyDown(Keycode::C)) {
		//contacts.clear();
		/*for (auto& body : bodies)
			body.orientation = 0.0f;*/
		/*for (auto& body : bodies)
			body.orientation = 0.0f;*/

		// TODO: Check if the contact count is correct.

		// Maybe an issue with hashing of the keys.

		// Clearing the contacts doesn't work.
		// The collision is not being detected so probably an issue with the detection?

		// Only happens on certain sides i think.

		// Below doesn't work
		/*for (auto& body : bodies)
			body.pos.y += 0.001f;*/
		// Also happens when boxes are not touching the static box.
		/*for (auto& body : bodies)
			body.pos.x += 0.001f;*/
		
	}

	static Body* selected;
	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		for (auto& body : bodies) {
			if (pointInBox(mousePos, body.pos, body.size, body.orientation))
				selected = &body;
		}
	}
	if (Input::isMouseButtonUp(MouseButton::LEFT)) {
		selected = nullptr;
	}

	if (selected != nullptr) {
		//selected->force = (mousePos - selected->pos) * 400.0f ;
		selected->force = (mousePos - selected->pos) * 400.0f * (mousePos - selected->pos).length() * 3;
	}

	//for (const auto& [_, contact] : contacts) {
	//	for (int i = 0; i < contact.numContacts; i++) {
	//		const auto& point = contact.contacts[i];
	//		/*Debug::drawPoint(point.position);
	//		Debug::drawRay(point.position, point.normal * -point.separation);*/
	//		contact.body2->pos += point.normal * -point.separation;
	//		break;
	//	}
	//}

	Debug::drawPoint(mousePos);

	for (auto& body : bodies) {
		if (body.isStatic())
			continue;

		body.vel += (body.force * body.invMass + gravity) * Time::deltaTime();
		body.force = Vec2{ 0.0f };

		body.angularVel = body.torque * body.invRotationalInertia * Time::deltaTime();
		body.torque = 0.0f;
	}

	doCollision();

	const auto invDeltaTime = 1.0f / Time::deltaTime();

	for (auto& [_, contact] : contacts) {
		contact.PreStep(invDeltaTime);
	}

	for (int i = 0; i < 10; i++) {
		for (auto& [_, contact] : contacts) {
			contact.ApplyImpulse();
		}
	}

	for (auto& body : bodies) {
		body.pos += body.vel * Time::deltaTime();
		body.orientation += body.angularVel * Time::deltaTime();

		body.force = Vec2{ 0.0f };
		body.torque = 0.0f;
	}

	for (const auto& body : bodies) {
		const auto rotate = Mat2::rotate(body.orientation);
		const auto edgeX = Vec2{ body.size.x, 0.0f } * rotate;
		const auto edgeY = Vec2{ 0.0f, body.size.y } * rotate;
		const auto vertex1 = (body.size / 2.0f) * rotate + body.pos;
		const auto vertex2 = vertex1 - edgeX;
		const auto vertex3 = vertex2 - edgeY;
		const auto vertex4 = vertex3 + edgeX;
		const auto color = &body == selected ? Vec3{ 1.0f, 0.0f, 0.0f } : Vec3{ 1.0f };
		Debug::drawLine(vertex1, vertex2, color);
		Debug::drawLine(vertex2, vertex3, color);
		Debug::drawLine(vertex3, vertex4, color);
		Debug::drawLine(vertex4, vertex1, color);
	}

	if (followedPos != nullptr) cameraPos = lerp(cameraPos, *followedPos, 2.0f * Time::deltaTime());

	renderer.update(gfx, cameraPos, cameraZoom);
}

bool Game2::warmStarting = true;
bool Game2::positionCorrection = true;
bool Game2::accumulateImpulses = true;