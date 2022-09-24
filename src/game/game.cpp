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
		.angularVel = 0.0f,
		.mass = PI<float> * 0.3f * 0.3f * 20.0f,
		.radius = 0.2f,
	});

	circleEntites.push_back(CircleEntity{
		.pos = Vec2{ 0.3f, 0.4f },
		.vel = Vec2{ 0.0f, 0.0f },
		.rotation = 0.3f,
		.angularVel = 0.0f,
		.mass = PI<float> * 0.3f * 0.3f * 20.0f,
		.radius = 0.3f,
	});

	//std::uniform_real_distribution random(-0.0f, 1.0f);
	//std::default_random_engine engine;
	//for (i32 i = 0; i < 20; i++) {
	//	circleEntites.push_back(CircleEntity{
	//		.pos = Vec2{ random(engine), random(engine) },
	//		.vel = Vec2{ 0.0f, 0.0f },
	//		.rotation = 0.3f,
	//		.angularVel = 0.0f,
	//		.mass = PI<float> * 0.3f * 0.3f,
	//		.radius = 0.07f,
	//	});
	//}
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
		circleEntites[0].angularVel += 0.2 + circleEntites[0].angularVel * 2.0f;
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
				static constexpr auto coefficientOfRestitution{ 0.90f };
				const auto rel{ b.vel - a.vel };
				const auto normal{ (a.pos - b.pos).normalized() };
				const Vec2 tangent{ normal.y, -normal.x };
				{
					/*
					TODO: Maybe name the function calculate collision linear impulse magnitude or something like that.
					v0 - (k / m0) * (n + t / r) * det(v0, n)
					Momentum is an instantenous change in velocity.
					P = mv
					v = P / m
					Post collision linear velocity
					v0' = v0 - kn / m0
					v1' = v1 + kn / m1
					Relative velocity
					v = v0 - v1
					v' = v0' - v1'
					
					Relative velocity perpendicular to the normal
					dot(v', n) = dot(-e * v, n)
					dot(v0' - v1', n) = dot(-e * v, n)
					dot(v0 - kn / m0 - (v1 + kn / m1), n) = dot(-e * v, n)
					dot(v0 - kn / m0 - v1 - kn / m1, n) = dot(-e * v, n)
					dot(v0 - v1 - kn(1 / m0  + 1 / m1), n) = dot(-e * v, n)
					-k(1 / m0 + 1 / m1) * dot(n, n) = -e * dot(v, n) - dot(v, n)
					-k(1 / m0 + 1 / m1) * dot(n, n) = (-e - 1) * dot(v, n)
					-k(1 / m0 + 1 / m1) * dot(n, n) = -(e + 1) * dot(v, n)
					k = (e + 1) * dot(v, n) / (k(1 / m0 + 1 / m1) * dot(n, n))

					a = angular velocity
					velocity of point particle = distance from center of mass * a
					// Using the same formula as for linear momentum. 
					L = angular momentum of point particle = velocity of point particle * mass
					v = L / (mass * radius)

					Variables with a prime refer to post collision variables.

					Both linear and angular velocity
					a = angular velocity
					v = linear velocity
					// TODO: This is kind of a bad name because it isn't tangent to the normal. Could name it r.
					// The length of the tangent is the distance from the center of mass to the hit point.
					t = tangent = vector from the center of mass to the hit point rotated by 90 deg

					Coefficient of restitution
					// Velocity is relative to object 0 in this formula but it could be reversed depending on the direction of the normal.
					e = (u1' - u0') / (u0 - u1) = -(u0' - u1') / (u0 - u1)) = post collision relative velocity / pre collision relative velocity 

					n = normal

					k = magnitude of impulse along the normal
					// k has to be divided by mass because the unit for an impulse is LMT^-1 so after the division it becomes LT^-1 which is just velocity. It has to be an impluse for momentum to be conserved.
					v0' = v0 + (k / m0)n
					v1' = v1 - (k / m1)n
					// Dividing by mass times distance because the units for angular momentum are different than for linear momentum. Look at the formula above there is a extra multiplication by length.
					a0' = a0 + k * dot(t, n) / m0r0
					a1' = a0 - k * dot(t, n) / m1r1
					// This formula says the if 2 circles collide there is no change in angular velocity (dot(t, n) always = 0). The rotation is only created by friction.

					u = velocity of the hit point point particle
					u0' = v0' + t0a0'
					u0' = v0 + (k / m0)n + (a0 + k * dot(t, n) / m0r0)t
					u0' = v0 + (k / m0)n + a0t + (k * dot(t, n) / m0r0)t
					u0' = v0 + a0t + (k / m0)n + (k * dot(t, n) / m0r0)t
					u0' = u0 + (k / m0)n + (k * dot(r0, n) / m0r0)r0
					u1' = u1 - (k / m1)n - (k * dot(r1, n) / m1r1)r1

					Post collision relative linear velocity
					dot(u0' - u1', n) = dot((u0' - u1') / (u0 - u1) * (u0 - u1), n)
					dot(u0' - u1', n) = dot(-e(u0 - u1), n)
					dot((u0 + (k / m0)n + (k * dot(r0, n) / m0r0)r0) - (u1 - (k / m1)n - (k * dot(r1, n) / m1r1)r1), n) = -e * dot(u0 - u1, n)
					dot(u0 + (k / m0)n + (k * dot(r0, n) / m0r0)r0 - u1 + (k / m1)n + (k * dot(r1, n) / m1r1)r1, n) = -e * dot(u0 - u1, n)
					dot(u0 - u1 + (k / m0)n + (k / m1)n + (k * dot(r0, n) / m0r0)r0 + (k * dot(r1, n) / m1r1)r1, n) = -e * dot(u0 - u1, n)
					dot(u0 - u1 + (1 / m0 + 1 / m1)kn + (k * dot(r0, n) / m0r0)r0 + (k * dot(r1, n) / m1r1)r1, n) = -e * dot(u0 - u1, n)
					dot(u0 - u1, n) + dot((1 / m0 + 1 / m1)kn + (k * dot(r0, n) / m0r0)r0 + (k * dot(r1, n) / m1r1)r1, n) = -e * dot(u0 - u1, n)
					dot((1 / m0 + 1 / m1)kn + (k * dot(r0, n) / m0r0)r0 + (k * dot(r1, n) / m1r1)r1, n) = -e * dot(u0 - u1, n) - dot(u0 - u1, n)
					dot((1 / m0 + 1 / m1)kn + (dot(r0, n) / m0r0)kr0 + (dot(r1, n) / m1r1)kr1, n) = (-e - 1) * dot(u0 - u1, n)
					k * dot((1 / m0 + 1 / m1)n + (dot(r0, n) / m0r0)r0 + (dot(r1, n) / m1r1)r1, n) = (-e - 1) * dot(u0 - u1, n)
					k = (-e - 1) * dot(u0 - u1, n) / (dot((1 / m0 + 1 / m1)n + (dot(r0, n) / m0r0)r0 + (dot(r1, n) / m1r1)r1, n))



					dot(v0 + (k / m0)n + (a0 + k * dot(t, n) / m0r0)t0 - (v1 - (k / m1)n + (a1 - k * dot(t, n) / m1r1)t1) , n) = dot(-e * (u0 - u1), n)
					dot(u0 + (k / m0)n + (a0 + k * dot(t, n) / m0r0)t0 - u1 + (k / m1)n - (a1 - k * dot(t, n) / m1r1)t1, n) = dot(-e * (u0 - u1), n)
					dot(u0 + (k / m0)n + (a0 + k * dot(t, n) / m0r0)t0 - u1 + (k / m1)n - (a1 - k * dot(t, n) / m1r1)t1, n) = dot(-e * (u0 - u1), n)
					dot(u0 - u1 + (k / m0)n + (k / m1)n + (a0 + k * dot(t, n) / m0r0)t0 - (a1 - k * dot(t, n) / m1r1)t1, n) = dot(-e * (u0 - u1), n)
					dot(u0 - u1 + (1 / m0 + 1 / m1)kn + (a0 + k * dot(t, n) / m0r0)t0 - (a1 - k * dot(t, n) / m1r1)t1, n) = dot(-e * (u0 - u1), n)
					dot(u0 - u1, n) + dot((1 / m0 + 1 / m1)kn + (a0 + k * dot(t, n) / m0r0)t0 - (a1 - k * dot(t, n) / m1r1)t1, n) = dot(-e * (u0 - u1), n)
					dot((1 / m0 + 1 / m1)kn + (a0 + k * dot(t, n) / m0r0)t0 - (a1 - k * dot(t, n) / m1r1)t1, n) = -e * dot(u0 - u1, n) - dot(u0 - u1, n)
					dot((1 / m0 + 1 / m1)kn + (a0 + k * dot(t, n) / m0r0)t0 - (a1 - k * dot(t, n) / m1r1)t1, n) = (-e - 1) * dot(u0 - u1, n)


					u0' = v0' + a0' * t
					u0' = v0 - kn / m0 + (a0 - k * det(v0, n) / m0r) * t
					u0' = v0 - kn / m0 + a0 * t - k * det(v0, n) / m0r * t
					u0' = v0 + t * a0 - kn / m0 - k * det(v0, n) / m0r * t
					u0' = u0 - kn / m0 - k * det(v0, n) / m0r * t
					u0' = u0 - (k / m0)(n - det(v0, n) / r * t)
					u1' = u1 + (k / m1)(n - det(v1, n) / r * t)

					dot(u0 - (k / m0)(n - det(v0, n) / r * t) - (u1 + (k / m1)(n - det(v1, n) / r * t)), n) = dot(-e * u, n)
					dot(u0 - (k / m0)(n - det(v0, n) / r * t) - u1 - (k / m1)(n - det(v1, n) / r * t), n) = dot(-e * u, n)
					dot(u0 - u1 - (k / m0)(n - det(v0, n) / r * t) - (k / m1)(n - det(v1, n) / r * t), n) = dot(-e * u, n)
					dot(u0 - u1, n) + dot(-(k / m0)(n - det(v0, n) / r * t) - (k / m1)(n - det(v1, n) / r * t), n) = dot(-e * u, n)
					dot(-(k / m0)(n - det(v0, n) / r * t) - (k / m1)(n - det(v1, n) / r * t), n) = -e * dot(u, n) - dot(u, n)
					-k * dot((1 / m0)(n - det(v0, n) / r * t) + (1 / m1)(n - det(v1, n) / r * t), n) = (-e - 1) * dot(u, n)
					-k * dot((1 / m0)(n - det(v0, n) / r * t) + (1 / m1)(n - det(v1, n) / r * t), n) = -(e + 1) * dot(u, n)
					k * dot((1 / m0)(n - det(v0, n) / r * t) + (1 / m1)(n - det(v1, n) / r * t), n) = (e + 1) * dot(u, n)
					k = ((e + 1) * dot(u, n)) / dot((1 / m0)(n - det(v0, n) / r * t) + (1 / m1)(n - det(v1, n) / r * t), n)


					u0' = v0' + kt / mr * det(v0, n)
					u0' = v0 - (k / m0) * (n + t / r) * det(v0, n)
					u1' = v1 + (k / m1) * (n + t / r) * det(v1, n)
					Relative
					u = u0 - u1
					u' = u0' - u1'

					dot(u', n) = dot(-e * u, n)

					// TODO: Fix the radii
					dot(v0 - (k / m0) * (n + t / r) * det(v0, n) - (v1 + (k / m1) * (n + t / r) * det(v1, n)), n) = dot(-e * u, n)
					dot(v0 - (k / m0) * (n + t / r) * det(v0, n) - v1 - (k / m1) * (n + t / r) * det(v1, n), n) = dot(-e * u, n)
					dot(v0 - v1 - (n + t / r) * ((k / m0) * det(v0, n) - (k / m1) * det(v1, n)), n) = dot(-e * u, n)
					dot(v0 - v1, n) - dot((n + t / r) * ((k / m0) * det(v0, n) - (k / m1) * det(v1, n)), n) = dot(-e * u, n)
					dot(v0 - v1, n) - dot((n + t / r) * ((k / m0) * det(v0, n) - (k / m1) * det(v1, n)), n) = dot(-e * u, n)

					t = tangent
					*/
					auto tangent = [](Vec2 v) -> Vec2 {
						return Vec2{ v.y, -v.x };
					};

					const auto hitPoint = a.pos + normal * a.radius;
					const auto ra = (hitPoint - a.pos).rotBy90deg();
					const auto rb = (hitPoint - b.pos).rotBy90deg();
					const Vec2 uRel{ (a.vel + ra * a.angularVel) - (b.vel + rb * b.angularVel) };

					const auto e = coefficientOfRestitution;
					const auto n = normal;

					const auto k = (-e - 1.0f) * dot(uRel, n) / (dot((1.0f / a.mass + 1.0f / b.mass) * n + (dot(ra, n) / a.mass * a.radius) * ra + (dot(rb, n) / b.mass * b.radius) * rb, n));

					a.vel += normal * (k / a.mass);
					b.vel -= normal * (k / b.mass);

					a.angularVel -= k * dot(ra, n) / (a.mass * a.radius);
					b.angularVel += k * dot(rb, n) / (b.mass * b.radius);

					a.pos += normal * 0.001f;
					b.pos -= normal * 0.001f;
				}
			}
		}
	}

	static constexpr float groundFriction{ 0.98f };
	for (auto& circle : circleEntites) {
		circle.pos += circle.vel * Time::deltaTime();
		circle.vel *= pow(groundFriction, Time::deltaTime());
		circle.rotation += circle.angularVel;
		circle.angularVel *= pow(0.94f, Time::deltaTime());
	}

	renderer.update(gfx);
}
