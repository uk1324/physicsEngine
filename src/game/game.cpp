#include <game/game.hpp>
#include <game/input.hpp>
#include <game/entities.hpp>
#include <engine/time.hpp>
#include <engine/window.hpp>
#include <math/utils.hpp>

#include <optional>
#include <random>

Game::Game(Gfx& gfx)
	: renderer(gfx) {
	Input::registerKeyButton(Keycode::W, GameButton::UP);
	Input::registerKeyButton(Keycode::S, GameButton::DOWN);
	Input::registerKeyButton(Keycode::A, GameButton::LEFT);
	Input::registerKeyButton(Keycode::D, GameButton::RIGHT);

	Input::registerKeyButton(Keycode::UP, GameButton::UP);
	Input::registerKeyButton(Keycode::DOWN, GameButton::DOWN);
	Input::registerKeyButton(Keycode::LEFT, GameButton::LEFT);
	Input::registerKeyButton(Keycode::RIGHT, GameButton::RIGHT);

	circleEntites.push_back(CircleEntity{
		.pos = Vec2{ 0.0f },
		.vel = Vec2{ 0.0f },
		.rotation = 0.0f,
		.rotationVel = 0.0f,
		.mass = PI<float> * 0.3f * 0.3f * 20.0f,
		.radius = 0.2f,
	});

	circleEntites.push_back(CircleEntity{
		.pos = Vec2{ 0.3f, 0.4f },
		.vel = Vec2{ 0.0f, 0.0f },
		.rotation = 0.3f,
		.rotationVel = 0.0f,
		.mass = PI<float> * 0.3f * 0.3f * 20.0f,
		.radius = 0.3f,
	});

	//std::uniform_real_distribution random(-0.0f, 1.0f);
	//std::default_random_engine engine;
	//for (i32 i = 0; i < 2; i++) {
	//	circleEntites.push_back(CircleEntity{
	//		.pos = Vec2{ random(engine), random(engine) },
	//		.vel = Vec2{ 0.0f, 0.0f },
	//		.rotation = 0.3f,
	//		.rotationVel = 0.0f,
	//		.mass = PI<float> * 0.3f * 0.3f,
	//		.radius = 0.07f,
	//	});
	//}
}

struct WallHit {
	float t;
	Vec2 normal;
};

// Assume only a single collision can happen at a time. A static vector would be good for handling multiple collisions.
static auto getWallCollisions(const CircleEntity& circle) -> std::optional<WallHit> {
	const Vec2 extents{ 1.0, Window::size().y / Window::size().x };
	static constexpr float EPSILON = 0.0001f;
	static constexpr Vec2 NORMALS[]{ Vec2{ 1.0f, 0.0f }, Vec2{ 0.0f, 1.0f } };
	for (i32 a = 0; a < 2; a++) {
		if (abs(circle.vel[a]) < EPSILON)
			continue;

		for (const auto sign : SIGNS) {
			float t = sign * (extents[a] - circle.pos[a] - circle.radius) / circle.vel[a];
			if (t > 0.0f && t < 1.0f) {
				return WallHit{ .t = sign * t, .normal = NORMALS[a] * sign };
			}
		}
	}
	return std::nullopt;
}

#include <utils/io.hpp>

auto Game::update(Gfx& gfx) -> void {
	Vec2 dir{ 0.0f };
	if (Input::isButtonHeld(GameButton::UP)) {
		dir.y += 1.0f;
	}
	if (Input::isButtonHeld(GameButton::DOWN)) {
		dir.y -= 1.0f;
	}

	if (Input::isButtonHeld(GameButton::RIGHT)) {
		dir.x += 1.0f;
	}
	if (Input::isButtonHeld(GameButton::LEFT)) {
		dir.x -= 1.0f;
	}
	circleEntites[0].vel += dir.normalized() * 0.5f * Time::deltaTime();

	auto start = circleEntites.begin();
	for (auto& a : circleEntites) {
		const Vec2 extents{ 1.0, Window::size().y / Window::size().x };
		for (i32 axis = 0; axis < 2; axis++) {
			if (a.pos[axis] - a.radius > extents[axis]) {
				a.pos[axis] -= extents[axis] * 2 + a.radius * 2;
			}

			if (a.pos[axis] + a.radius < -extents[axis]) {
				a.pos[axis] += extents[axis] * 2 + a.radius * 2;
			}
		}

		start++;
		for (auto it = start; it != circleEntites.end(); it++) {
			auto& b = *it;
			if ((a.pos - b.pos).lengthSq() < (a.radius + b.radius) * (a.radius + b.radius)) {
				// P = mv

				auto getK = [&a, &b](Vec2 vel, float coefficientOfRestitution, Vec2 normal) {
					return dot(vel * (coefficientOfRestitution + 1.0f), normal)
						/ dot(normal * (1.0f / a.mass + 1.0f / b.mass), normal);
				};


				auto rel = b.vel - a.vel;
				auto normal = (a.pos - b.pos).normalized();
				float k = getK(rel, 0.90f, normal);
				a.vel += normal * (k / a.mass);
				a.pos += normal * 0.001f;

				b.vel += -normal * (k / b.mass);
				b.pos += -normal * 0.001f;

				//b.vel = normal * (getK(-b.vel, 0.9f, -normal) / b.mass);
				//b.rotation = atan2(normal.y, normal.x);
				//b.vel += normal * (getK(b.vel, 0.9f, normal) / b.mass);
				/*a.vel += normal * dot(normal, b.vel) / a.mass * 0.01f;
				b.vel += normal * dot(normal, a.vel) / b.mass * 0.01f;*/
				//a.rotationVel -= (1.0f - dot(normal, b.vel)) * (1.0f / a.radius) / 1000.0f;
				//b.rotationVel += (1.0f - dot(normal, b.vel)) * (1.0f / b.radius) / 1000.0f;
				//a.vel += normal;
				//b.vel -= normal;
			}
		}


		//const auto collision = getWallCollisions(entity);
		//if (!collision)
		//	continue;

		//entity.vel *= (1.0f - collision->t) - 0.01f;
		//entity.pos += entity.vel;
		//entity.vel = Vec2{ 0.0f };
		//entity.vel -= collision->normal * dot(entity.vel, collision->normal);
		//	if (entity.pos[a] + entity.vel[a] + entity.radius >= extents[a]) {
		//		entity.pos[a] = extents[a] - entity.radius;
		//		entity.vel[a] = -entity.vel[a] / 2.0f;
		//	}

		//	if (entity.pos[a] + entity.vel[a] - entity.radius <= -extents[a]) {
		//		entity.pos[a] = -extents[a] + entity.radius;
		//		entity.vel[a] = -entity.vel[a] / 2.0f;
		//	}
		//}
	}

	static constexpr float groundFriction = 0.98f;
	for (auto& circle : circleEntites) {
		circle.pos += circle.vel * Time::deltaTime();
		circle.vel *= pow(groundFriction, Time::deltaTime());
		circle.rotation += circle.rotationVel;
		circle.rotationVel *= pow(0.94f, Time::deltaTime());
	}

	renderer.update(gfx);
}
