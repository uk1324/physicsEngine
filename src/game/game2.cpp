#include <game/game2.hpp>
#include <game/body.hpp>
#include <game/debug.hpp>
#include <game/input.hpp>
#include <game/collision/collision.hpp>
#include <engine/time.hpp>
#include <engine/window.hpp>
#include <math/utils.hpp>
#include <math/mat2.hpp>

// TODO: Understand gauss–Seidel method
// spatial hashing / bucketing
// inactive flag on objects

// The exact number of collisions checked in the O(n^2) broadphase is choose(n, 2).


CollisionMap contacts;
std::vector<Body> bodies;

Game2::Game2(Gfx& gfx)
	: renderer{ gfx } {

	int height = 4;
	float boxSize = 1.0f;
	float gapSize = 0.1f;
	for (int i = 1; i < height + 1; i++) {
		for (int j = 0; j < i; j++) {
			float y = (height + 1 - i) * (boxSize + gapSize);
			float x = -i * (boxSize / 2.0f + boxSize / 8.0f) + j * (boxSize + boxSize / 4.0f);

			bodies.push_back(Body{ Vec2{ x, y }, BoxCollider{ Vec2{ boxSize } }, false });
			//bodies.push_back(Body{ Vec2{ x, y }, CircleCollider{ boxSize / 2.0f }, false });
		}
	}
	bodies.push_back(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 100.0f } }, true });
	camera.zoom = 0.125f;
	
	bodies.push_back(Body{ Vec2{ 0.0f, 10.0f }, CircleCollider{ 0.5f }, false });
	bodies.push_back(Body{ Vec2{ 0.0f, 7.0f }, CircleCollider{ 0.5f }, false });

	static std::vector<Body*> vAdd;
	for (auto& body : bodies) {
		vAdd.push_back(&body);
	}
	static const std::vector<Body*> vDelete;
	collisionSystem.update(vAdd, vDelete);

	Window::maximize();
	followedPos = &bodies[0].pos;
	controlledValue = &bodies[0].vel;
	gravity = Vec2{ 0.0f, -10.0f };
	//gravity = Vec2{ 0.0f, 0.0f };
}

auto doCollision() -> void {
	auto start{ bodies.begin() };
	for (auto& a : bodies) {
		start++;
		for (auto it = start; it != bodies.end(); it++) {
			auto& b{ *it };

			if (a.isStatic() && b.isStatic())
				continue;

			CollisionKey key{ &a, &b };

			// TODO: Can be made const.
			if (auto collision = collide(a.pos, a.orientation, a.collider, b.pos, b.orientation, b.collider); collision.has_value()) {
				// TODO: Move this into some function or constructor probably when making a better collision system.
				collision->coefficientOfFriction = sqrt(a.coefficientOfFriction * b.coefficientOfFriction);
				if (const auto& oldContact = contacts.find(key); oldContact == contacts.end()) {
					contacts[key] = *collision;
				} else {
					oldContact->second.Update(collision->contacts, collision->numContacts);
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
	/*const auto color = &body == selected ? Vec3{ 1.0f, 0.0f, 0.0f } : Vec3{ 1.0f };*/
	const auto color = Vec3{ 1.0f };
	Debug::drawLine(vertex1, vertex2, color);
	Debug::drawLine(vertex2, vertex3, color);
	Debug::drawLine(vertex3, vertex4, color);
	Debug::drawLine(vertex4, vertex1, color);
};

auto Game2::detectCollisions() -> void {
	static const std::vector<Body*> v;
	collisionSystem.update(v, v);
	collisionSystem.detectCollisions(contacts);
}

auto Game2::update(Gfx& gfx) -> void {
	const auto mousePos = camera.screenSpaceToCameraSpace(Input::cursorPos());

	if (Input::isKeyDown(Keycode::G)) {
		__debugbreak();
	}

	BoxCollider collider{ .size = Vec2{ 1.0f, 2.0f } };
	Vec2 pos{ 3.0f };
	static auto elapsed = 0.0f;
	elapsed += Time::deltaTime();
	const auto orientation = elapsed;

	const auto rayBegin = Vec2{ 0.0f };
	const auto rayEnd = mousePos * 10.0f;
	const auto hit = raycast(rayBegin, rayEnd, collider, pos, orientation);
	drawBox(collider.size, pos, orientation);
	if (hit.has_value()) {
		Debug::drawRay(rayBegin, (rayEnd - rayBegin) * hit->t);
	}
	else {
		Debug::drawRay(rayBegin, rayEnd);
	}


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

	Debug::drawPoint(mousePos);

	for (auto& body : bodies) {
		if (body.isStatic())
			continue;

		// It might be better to use impulses instead of forces so they are independent of time.
		body.vel += (body.force * body.invMass + gravity) * Time::deltaTime();
		body.force = Vec2{ 0.0f };

		body.angularVel = body.torque * body.invRotationalInertia * Time::deltaTime();
		body.torque = 0.0f;
	}

	//doCollision();
	detectCollisions();

	const auto invDeltaTime = 1.0f / Time::deltaTime();

	for (auto& [key, contact] : contacts) {
		contact.PreStep(key.body1, key.body2, invDeltaTime);
	}

	for (int i = 0; i < 10; i++) {
		for (auto& [key, contact] : contacts) {
			contact.ApplyImpulse(key.body1, key.body2);
		}
	}

	for (auto& body : bodies) {
		body.pos += body.vel * Time::deltaTime();
		body.orientation += body.angularVel * Time::deltaTime();
	}

	for (const auto& body : bodies) {
		if (const auto box = std::get_if<BoxCollider>(&body.collider); box != nullptr) {
			drawBox(box->size, body.pos, body.orientation);
		} else if (const auto circle = std::get_if<CircleCollider>(&body.collider); circle != nullptr) {
			Debug::drawEmptyCircle(body.pos, circle->radius, body.orientation);
		}
		//Debug::drawAabb(aabb(body.collider, body.pos, body.orientation), Vec3::RED);
	}

	camera.aspectRatio = Window::size().x / Window::size().y;

	//if (followedPos != nullptr) cameraPos = lerp(cameraPos, *followedPos, 2.0f * Time::deltaTime());
	if (followedPos != nullptr) camera.interpolateTo(*followedPos, 2.0f * Time::deltaTime());

	renderer.update(gfx, camera);
}	

bool Game2::warmStarting = true;
bool Game2::positionCorrection = true;
bool Game2::accumulateImpulses = true;