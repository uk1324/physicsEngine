#include <game/game.hpp>
#include <game/input.hpp>
#include <engine/time.hpp>
#include <engine/window.hpp>
#include <math/utils.hpp>

#include <optional>
#include <random>

Game::Game(Gfx& gfx)
	: renderer{ gfx }
	, material0{ .bounciness = 0.9f }
	, material1{ .bounciness = 0.1f } {
	Input::registerKeyButton(Keycode::W, GameButton::UP);
	Input::registerKeyButton(Keycode::S, GameButton::DOWN);
	Input::registerKeyButton(Keycode::A, GameButton::LEFT);
	Input::registerKeyButton(Keycode::D, GameButton::RIGHT);

	Input::registerKeyButton(Keycode::UP, GameButton::UP);
	Input::registerKeyButton(Keycode::DOWN, GameButton::DOWN);
	Input::registerKeyButton(Keycode::LEFT, GameButton::LEFT);
	Input::registerKeyButton(Keycode::RIGHT, GameButton::RIGHT);

	//circleEntites.push_back(CircleEntity{
	//	.pos = Vec2{ 0.0f },
	//	.vel = Vec2{ 0.0f },
	//	.rotation = 0.0f,
	//	.angularVel = 0.0f,
	//	.mass = PI<float> * 0.3f * 0.3f * 20.0f,
	//	.radius = 0.2f,
	//	.material = &material0
	//});

	//circleEntites.push_back(CircleEntity{
	//	.pos = Vec2{ 0.3f, 0.4f },
	//	.vel = Vec2{ 0.0f, 0.0f },
	//	.rotation = 0.3f,
	//	.angularVel = 0.0f,
	//	.mass = PI<float> * 0.3f * 0.3f * 20.0f,
	//	.radius = 0.3f,
	//	.material = &material0
	//});

	std::uniform_real_distribution random(-0.0f, 1.0f);
	std::default_random_engine engine;
	for (i32 i = 0; i < 20; i++) {
		circleEntites.push_back(CircleEntity{
			.pos = Vec2{ random(engine), random(engine) },
			.vel = Vec2{ 0.0f, 0.0f },
			.rotation = 0.3f,
			.angularVel = 0.0f,
			.mass = PI<float> * 0.3f * 0.3f,
			.radius = 0.07f,
			.material = &material0
		});
	}
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

	if (Input::isKeyDown(Keycode::R)) {
		circleEntites[0].angularVel += 0.2f + circleEntites[0].angularVel * 2.0f;
	}

	auto start{ circleEntites.begin() };
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
			auto& b{ *it };
			if ((a.pos - b.pos).lengthSq() < (a.radius + b.radius) * (a.radius + b.radius)) {
				/*
				Variables with a prime refer to post collision variables.

				Both linear and angular velocity
				n = collision normal

				k = magnitude of the collision impulse along the normal

				v = linear velocity
				p = momentum = mv
				// k has to be divided by mass because the unit for an impulse is LMT^-1 so after the division it becomes LT^-1 which is just velocity. It has to be an impluse for momentum to be conserved.
				v0' = v0 + (k / m0)n
				v1' = v1 - (k / m1)n

				a = angular velocity
				// The same formula as for linear momentum is used.
				L = angular momentum of point particle = velocity of point particle * mass
				a = L / (mass * radius)
				// Dividing by mass times distance because the units for angular momentum are different than for linear momentum. Look at the formula above. There is a extra multiplication by length.
				l = distance from the center of mass to particle
				a0' = a0 + k * dot(r0, n) / m0l0
				a1' = a0 - k * dot(r0, n) / m1l1
				// The above formula states the if 2 circles collide there is no change in angular velocity (dot(r, n) always = 0 because the angle between the is always 90deg). The rotation is only created by friction.

				r = l rotated by 90deg
				// The further away from the center of mass the faster it spins.
				u = velocity of point particle = v + r * a
				u0' = v0' + r0a0'
				u0' = v0 + (k / m0)n + (a0 + k * dot(r0, n) / m0l0)r0
				u0' = v0 + (k / m0)n + a0t + (k * dot(r0, n) / m0l0)r0
				u0' = v0 + a0t + (k / m0)n + (k * dot(r0, n) / m0l0)r0
				u0' = u0 + (k / m0)n + (k * dot(r0, n) / m0l0)r0
				u1' = u1 - (k / m1)n - (k * dot(r0, n) / m1l1)r1

				Coefficient of restitution
				// The order is based of the direction of the normal.
				e = (u1' - u0') / (u0 - u1) = -(u0' - u1') / (u0 - u1)) = post collision relative velocity / pre collision relative velocity

				Post collision relative linear velocity
				dot(u0' - u1', n) = dot((u0' - u1') / (u0 - u1) * (u0 - u1), n)
				dot(u0' - u1', n) = dot(-e(u0 - u1), n)
				dot((u0 + (k / m0)n + (k * dot(r0, n) / m0l0)r0) - (u1 - (k / m1)n - (k * dot(r1, n) / m1l1)r1), n) = -e * dot(u0 - u1, n)
				dot(u0 + (k / m0)n + (k * dot(r0, n) / m0l0)r0 - u1 + (k / m1)n + (k * dot(r1, n) / m1l1)r1, n) = -e * dot(u0 - u1, n)
				dot(u0 - u1 + (k / m0)n + (k / m1)n + (k * dot(r0, n) / m0l0)r0 + (k * dot(r1, n) / m1l1)r1, n) = -e * dot(u0 - u1, n)
				dot(u0 - u1 + (1 / m0 + 1 / m1)kn + (k * dot(r0, n) / m0l0)r0 + (k * dot(r1, n) / m1l1)r1, n) = -e * dot(u0 - u1, n)
				dot(u0 - u1, n) + dot((1 / m0 + 1 / m1)kn + (k * dot(r0, n) / m0l0)r0 + (k * dot(r1, n) / m1l1)r1, n) = -e * dot(u0 - u1, n)
				dot((1 / m0 + 1 / m1)kn + (k * dot(r0, n) / m0l0)r0 + (k * dot(r1, n) / m1l1)r1, n) = -e * dot(u0 - u1, n) - dot(u0 - u1, n)
				dot((1 / m0 + 1 / m1)kn + (dot(r0, n) / m0l0)kr0 + (dot(r1, n) / m1l1)kr1, n) = (-e - 1) * dot(u0 - u1, n)
				k * dot((1 / m0 + 1 / m1)n + (dot(r0, n) / m0l0)r0 + (dot(r1, n) / m1l1)r1, n) = (-e - 1) * dot(u0 - u1, n)
				k = (-e - 1) * dot(u0 - u1, n) / (dot((1 / m0 + 1 / m1)n + (dot(r0, n) / m0l0)r0 + (dot(r1, n) / m1l1)r1, n))
				*/

				// The coefficient of restitution is a property measured between two materials. The average of bounciness is only an approximation. For it to be correct it would need to be a lookup table with 2 materials as keys.
				const auto coefficientOfRestitution{ (a.material->bounciness + b.material->bounciness) / 2.0f };
				const auto normal{ (a.pos - b.pos).normalized() };
				const auto hitPoint = a.pos + normal * a.radius;
				const auto ra = (hitPoint - a.pos).rotBy90deg();
				const auto rb = (hitPoint - b.pos).rotBy90deg();
				const Vec2 uRel{ (a.vel + a.angularVel * ra) - (b.vel + b.angularVel * rb) };

				const auto k = (-coefficientOfRestitution - 1.0f) * dot(uRel, normal) / (dot((1.0f / a.mass + 1.0f / b.mass) * normal + (dot(ra, normal) / a.mass * a.radius) * ra + (dot(rb, normal) / b.mass * b.radius) * rb, normal));

				a.vel += normal * (k / a.mass);
				b.vel -= normal * (k / b.mass);

				a.angularVel -= k * dot(ra, normal) / (a.mass * a.radius);
				b.angularVel += k * dot(rb, normal) / (b.mass * b.radius);

				a.pos += normal * 0.001f;
				b.pos -= normal * 0.001f;
			}
		}
	}

	static constexpr float groundFriction{ 0.98f };
	static constexpr float gravity = 10.0f;
	for (auto& circle : circleEntites) {
		circle.pos += circle.vel * Time::deltaTime();
		//circle.vel.y -= Time::deltaTime() * 10.0f;
		circle.vel *= pow(groundFriction, Time::deltaTime());
		circle.rotation += circle.angularVel;
		//circle.angularVel *= pow(0.94f, Time::deltaTime());
	}

	renderer.update(gfx);
}
