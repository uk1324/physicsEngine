#include <game/game.hpp>
#include <game/input.hpp>
#include <engine/time.hpp>
#include <engine/window.hpp>
#include <game/debug.hpp>
#include <math/utils.hpp>

#include <optional>
#include <random>

Game::Game(Gfx& gfx)
	: renderer{ gfx }
	, material0{ .bounciness = 0.9f }
	, material1{ .bounciness = 0.1f } {
	Window::maximize();

	Input::registerKeyButton(Keycode::W, GameButton::UP);
	Input::registerKeyButton(Keycode::S, GameButton::DOWN);
	Input::registerKeyButton(Keycode::A, GameButton::LEFT);
	Input::registerKeyButton(Keycode::D, GameButton::RIGHT);

	Input::registerKeyButton(Keycode::UP, GameButton::UP);
	Input::registerKeyButton(Keycode::DOWN, GameButton::DOWN);
	Input::registerKeyButton(Keycode::LEFT, GameButton::LEFT);
	Input::registerKeyButton(Keycode::RIGHT, GameButton::RIGHT);
	
	//circleEntites.push_back(CircleEntity{
	//	.transform = { Vec2{ 0.0f, 0.0f }, 0.0f },
	//	.collider = /* CircleCollider (causes some interal compiler error */ { .radius = 0.2f },
	//	.physics = PhysicsInfo{ &material0, PI<float> * 0.2f * 0.2f * 20.0f }
	//});

	//circleEntites.push_back(CircleEntity{
	//	.transform = { Vec2{ 0.3f, 0.4f }, 0.0f },
	//	.collider = { .radius = 0.3f },
	//	.physics = PhysicsInfo{ &material0, PI<float> * 0.3f * 0.3f * 20.0f },
	//});

	circleEntites.push_back(CircleEntity{
		.transform = { Vec2{ 0.4f, 0.3f }, 0.0f },
		.collider = /* CircleCollider (causes some interal compiler error */ {.radius = 0.2f },
		.physics = PhysicsInfo{ &material0, PI<float> * 0.2f * 0.2f * 20.0f }
	});

	//circleEntites.push_back(CircleEntity{
	//	.transform = { Vec2{ 0.3f, 0.4f }, 0.0f },
	//	.collider = {.radius = 0.3f },
	//	.physics = PhysicsInfo{ &material0, PI<float> * 0.3f * 0.3f * 20.0f },
	//	});

	lineEntites.push_back(LineEntity{
		.transform = { Vec2{ 0.4f, 0.1f }, 0.2f },
		.collider = { .halfLength = 0.4f },
		.physics = PhysicsInfo{ &material0, PI<float> * 0.3f * 0.3f * 20.0f },
	});

	lineEntites.push_back(LineEntity{
		.transform = { Vec2{ -0.4f, -0.4f }, -0.2f },
		.collider = {.halfLength = 0.8f },
		.physics = PhysicsInfo{ &material0, PI<float> * 0.3f * 0.3f * 20.0f },
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
	//		.material = &material0
	//	});
	//}
}

#include <utils/io.hpp>

static auto collisionResponse(
	Vec2 hitPoint, 
	Vec2 hitNormal, 
	Transform& aTransform, 
	Transform& bTransform, 
	PhysicsInfo& aPhysics, 
	PhysicsInfo& bPhysics) -> void {

	/*
	P = Ft = mv
	F = P/t
	F = ma
	a = F / m = (P / t) / m = (m0v / tm1)

	Assume that the friction is applied for the whole frame.
	Problems might arise when doing time of impact calculations because the friction would not be applied for long enough or would be applied to quickly.
	*/
	const auto ra = (hitPoint - aTransform.pos).rotBy90deg();
	const auto rb = (hitPoint - bTransform.pos).rotBy90deg();

	const auto parallel = hitNormal.rotBy90deg().normalized();
	const auto aVel = -ra * aPhysics.angularVel + aPhysics.vel;
	const auto bVel = -rb * bPhysics.angularVel + bPhysics.vel;
	const auto aForce = (dot(1.0f / aPhysics.invMass * aVel, parallel) * parallel) * 0.9f;
	const auto bForce = (dot(1.0f / bPhysics.invMass * bVel, parallel) * parallel) * 0.9f;

	Debug::drawLine(aTransform.pos, aTransform.pos - aForce * Time::deltaTime() * 10.0f);

	aPhysics.vel -= bForce * Time::deltaTime();
	bPhysics.vel -= aForce * Time::deltaTime();
	aPhysics.angularVel += det(hitNormal, aForce) * Time::deltaTime();
	bPhysics.angularVel += det(hitNormal, bForce) * Time::deltaTime();

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
	const auto coefficientOfRestitution{ (aPhysics.material->bounciness + bPhysics.material->bounciness) / 2.0f };
	const auto la = ra.length();
	const auto lb = rb.length();
	const Vec2 uRel{ (aPhysics.vel + aPhysics.angularVel * ra) - (bPhysics.vel + bPhysics.angularVel * rb) };

	const auto k = (-coefficientOfRestitution - 1.0f) * dot(uRel, hitNormal) / (dot((1.0f * aPhysics.invMass + 1.0f * bPhysics.invMass) * hitNormal + (dot(ra, hitNormal) * aPhysics.invMass / la) * ra + (dot(rb, hitNormal) * bPhysics.invMass / lb) * rb, hitNormal));

	aPhysics.vel += hitNormal * (k * aPhysics.invMass);
	bPhysics.vel -= hitNormal * (k * bPhysics.invMass);

	aPhysics.angularVel -= k * dot(ra, hitNormal) * aPhysics.invMass / la;
	bPhysics.angularVel += k * dot(rb, hitNormal) * bPhysics.invMass / lb;

	//aTransform.pos += hitNormal * 0.001f;
	//bTransform.pos -= hitNormal * 0.001f;
}

static auto integrate(Transform& transform, PhysicsInfo& physics) {
	static constexpr float groundFriction{ 0.98f };
	static constexpr float gravity = 10.0f;
	transform.pos += physics.vel * Time::deltaTime();
	/*
	Gravity is a constant acceleration because the formula for gravity is 
	F = G(m1m2/r^2)
	F = ma
	a = F/m 
	a = G(m1m2/r^2) / m1 = Gm1m2/r^2m1 = Gm2/r^2
	*/
	physics.vel.y -= Time::deltaTime() * 1.0f;
	physics.vel *= pow(groundFriction, Time::deltaTime());
	transform.orientation += physics.angularVel * Time::deltaTime();
	//circle.angularVel *= pow(0.94f, Time::deltaTime());
}

auto Game::update(Gfx& gfx) -> void {
	//Vec2 dir{ 0.0f };
	//if (Input::isButtonHeld(GameButton::UP)) {
	//	dir.y += 1.0f;
	//}
	//if (Input::isButtonHeld(GameButton::DOWN)) {
	//	dir.y -= 1.0f;
	//}

	//if (Input::isButtonHeld(GameButton::RIGHT)) {
	//	dir.x += 1.0f;
	//}
	//if (Input::isButtonHeld(GameButton::LEFT)) {
	//	dir.x -= 1.0f;
	//}
	////circleEntites[0].physics.vel += dir.normalized() * 0.5f * Time::deltaTime();
	//lineEntites[0].physics.vel += dir.normalized() * 0.5f * Time::deltaTime();

	//if (Input::isKeyDown(Keycode::R)) {
	//	circleEntites[0].physics.angularVel += 0.2f + circleEntites[0].physics.angularVel * 2.0f;
	//}

	//{
	//	auto start{ circleEntites.begin() };
	//	for (auto& a : circleEntites) {
	//		start++;
	//		for (auto it = start; it != circleEntites.end(); it++) {
	//			auto& b{ *it };
	//			if ((a.transform.pos - b.transform.pos).lengthSq() < pow(a.collider.radius + b.collider.radius, 2)) {
	//				const auto normal{ (a.transform.pos - b.transform.pos).normalized() };
	//				const auto hitPoint = a.transform.pos + normal * a.collider.radius;
	//				collisionResponse(hitPoint, normal, a.transform, b.transform, a.physics, b.physics);
	//			}
	//		}
	//		for (auto& b : lineEntites) {
	//			/*const Line line{ Vec2::oriented(b.transform.orientation), 0.0 };
	//			b.transform.pos*/
	//			Line line{ Vec2::oriented(b.transform.orientation).rotBy90deg(), 0.0 };
	//			line = line.translated(b.transform.pos);
	//			line.n = line.n.normalized();
	//			const auto centerOffsetAlongLine = det(line.n, b.transform.pos);

	//			const auto signedDistance = ::signedDistance(line, a.transform.pos);
	//			if (abs(signedDistance) <= a.collider.radius) {
	//				const auto offsetAlongLine = det(line.n, a.transform.pos);
	//				// Can't just check if the offsetAlongLine is in the range because this would only work if this was an OBB oriented in the same way as the line and not a sphere.
	//				
	//				const auto vectorAlongLine = line.n.rotBy90deg();
	//				/*const auto possibleHitPoint = vectorAlongLine * std::clamp(offsetAlongLine, centerOffsetAlongLine - b.collider.halfLength, centerOffsetAlongLine + b.collider.halfLength) + line.n * dot(line.n, b.transform.pos);*/

	//				const auto hitNormal = (signedDistance < 0) ? line.n.normalized() : -line.n.normalized();
	//				const auto possibleHitPoint = a.transform.pos - hitNormal * a.collider.radius;
	//				if (const auto hitNonCorner = offsetAlongLine > centerOffsetAlongLine - b.collider.halfLength
	//					&& offsetAlongLine < centerOffsetAlongLine + b.collider.halfLength) {
	//					collisionResponse(possibleHitPoint, hitNormal, a.transform, b.transform, a.physics, b.physics);
	//					// @Hack
	//					a.transform.pos += (a.collider.radius - abs(signedDistance)) * hitNormal;
	//				} else {
	//					// TODO: This code might be broken because of the sign change in Line. Check it.
	//					const auto corner = -vectorAlongLine * std::clamp(offsetAlongLine, centerOffsetAlongLine - b.collider.halfLength, centerOffsetAlongLine + b.collider.halfLength) - line.n * line.d;
	//					const auto distanceFromCorner = distance(corner, a.transform.pos);
	//					const auto normal = (a.transform.pos - corner).normalized();
	//					if (const auto hitCorner = distanceFromCorner <= a.collider.radius) {
	//						collisionResponse(corner, normal, a.transform, b.transform, a.physics, b.physics);
	//						// @Hack
	//						a.transform.pos += (a.collider.radius - (corner - a.transform.pos).length()) * normal;
	//					}
	//				}
	//			}
	//		}
	//	}
	//}
	//
	//// @Hack
	//for (auto& line : lineEntites) {
	//	line.physics.angularVel = 0.0f;
	//	line.physics.vel = Vec2{ 0.0f };
	//	//line.physics.vel.y += Time::deltaTime() * 1.0f * 0.98f;
	//}

	//for (auto& circle : circleEntites) integrate(circle.transform, circle.physics);
	////for (auto& line : lineEntites) integrate(line.transform, line.physics);

	auto draw = [this](const std::vector<Vec2>& vertices, Vec2 pos, Vec3 color) {
		for (size_t i = 0; i < vertices.size(); i++) {
			Debug::drawLine((vertices[i] + pos), (i + 1 < vertices.size() ? vertices[i + 1] : vertices[0]) + pos, color);
		}
	};

	std::vector<Vec2> a{ Vec2{ -0.1f, -0.3f }, Vec2{ -0.1f, 0.2f }, Vec2{ 0.05f, 0.2f }, Vec2{ 0.3f, -0.1f } };
	
	Vec2 aPos{ renderer.mousePosToScreenPos(Input::cursorPos()) };
	for (auto& v : a) {
		v += aPos;
	}
	const std::vector<Vec2> b{ Vec2{ -0.2f, -0.3f }, Vec2{ -0.2f, 0.1f }, Vec2{ 0.01f, 0.2f }, Vec2{ 0.2f, -0.1f } };
	Vec2 bPos{ 0.0f };

	auto support = [](Vec2 dir, const std::vector<Vec2>& vertices, Vec2 pos) -> Vec2 {
		ASSERT(vertices.size() != 0);
		auto pointFurthestOnPolygonInDirection = vertices[0] + pos;
		auto distance = dot(pointFurthestOnPolygonInDirection, dir);
		for (usize i = 1; i < vertices.size(); i++) {
			if (const auto thisDistance = dot(vertices[i] + pos, dir); thisDistance > distance) {
				distance = thisDistance;
				pointFurthestOnPolygonInDirection = vertices[i] + pos;
			}
		}
		return pointFurthestOnPolygonInDirection;
	};

	auto minkowskiDifferenceSupport = [support](Vec2 dir, const std::vector<Vec2>& a, const std::vector<Vec2>& b) -> Vec2 {
		return -(support(dir, a, Vec2{ 0.0f }) - support(-dir, b, Vec2{ 0.0f }));
	};

	Vec2 direction{ 1.0f, 0.0f };
	std::vector<Vec2> simplex{ minkowskiDifferenceSupport(direction, a, b) };

	if (Input::isKeyDown(Keycode::D)) {
		__debugbreak();
	}

	bool collision = false;
	Vec2 closestPointOnSimplexToOrigin;
	for (;;) {
		struct LineBarycentric {
			float u, v;
		};
		auto lineBarycentric = [](Vec2 a, Vec2 b) -> LineBarycentric {
			// Line barycentric coordinate for the origin projected onto the line a b.
			// Vec2{ 0.0f } = u * b + v * a
			const auto normalize = 1.0f / pow(distance(a, b), 2);
			return LineBarycentric{
				.u = dot(b - a, /* Vec2{ 0.0f } */ -a) * normalize,
				.v = dot(a - b, /* Vec2{ 0.0f } */ -b) * normalize,
			};
		};

		{
			if (simplex.size() == 1) {
				closestPointOnSimplexToOrigin = simplex[0];
				const auto newSupport = minkowskiDifferenceSupport(closestPointOnSimplexToOrigin, a, b);
				simplex.push_back(newSupport);
			} else if (simplex.size() == 2) {
				const auto [u, v] = lineBarycentric(simplex[0], simplex[1]);
				if (u < 0.0f) {
					closestPointOnSimplexToOrigin = simplex[0];
					const auto newSupport = minkowskiDifferenceSupport(closestPointOnSimplexToOrigin, a, b);
					if (std::find(simplex.begin(), simplex.end(), newSupport) != simplex.end()) {
						break;
					}
					else {
						simplex.erase(simplex.begin() + 1);
					}
				}
				else if (v < 0.0f) {
					closestPointOnSimplexToOrigin = simplex[1];
					const auto newSupport = minkowskiDifferenceSupport(closestPointOnSimplexToOrigin, a, b);
					if (std::find(simplex.begin(), simplex.end(), newSupport) != simplex.end()) {
						break;
					}
					else {
						simplex.erase(simplex.begin() + 0);
					}
				}
				else {
					closestPointOnSimplexToOrigin = u * simplex[1] + v * simplex[0];
					const auto newSupport = minkowskiDifferenceSupport(closestPointOnSimplexToOrigin, a, b);
					if (std::find(simplex.begin(), simplex.end(), newSupport) != simplex.end()) {
						break;
					}
					else {
						simplex.push_back(newSupport);
					}
				};
			} else {
				const auto
					e0 = simplex[1] - simplex[0],
					e1 = simplex[2] - simplex[1],
					e2 = simplex[0] - simplex[2];
				const auto
					l0 = /* Vec2{ 0.0f } */ -simplex[0],
					l1 = /* Vec2{ 0.0f } */ -simplex[1],
					l2 = /* Vec2{ 0.0f } */ -simplex[2];
				const auto area = det(e0, e1);
				if (area == 0.0f) {
					// Don't think this should happen but it does.
					break;
				}
				const auto
					uabc = det(e0, l0) / area,
					vabc = det(e1, l1) / area,
					wabc = det(e2, l2) / area;

				if (uabc > 0.0f && vabc > 0.0f && wabc > 0.0f) {
					collision = true;
					break;
				}

				const auto [uab, vab] = lineBarycentric(simplex[0], simplex[1]);
				const auto [ubc, vbc] = lineBarycentric(simplex[1], simplex[2]);
				const auto [uca, vca] = lineBarycentric(simplex[2], simplex[0]);

				auto vertexCase = [&](usize pointIndex, usize remove1, usize remove2) {
					closestPointOnSimplexToOrigin = simplex[pointIndex];
					const auto newSupport = minkowskiDifferenceSupport(closestPointOnSimplexToOrigin, a, b);
					if (std::find(simplex.begin(), simplex.end(), newSupport) != simplex.end()) {
						return true;
					} else {
						simplex.erase(simplex.begin() + remove1);
						simplex.erase(simplex.begin() + remove2);
					}
					return false;
				};

				auto edgeCase = [&](Vec2 point, usize remove) {
					closestPointOnSimplexToOrigin = point;
					const auto newSupport = minkowskiDifferenceSupport(closestPointOnSimplexToOrigin, a, b);
					if (std::find(simplex.begin(), simplex.end(), newSupport) != simplex.end()) {
						return true;
					}
					else {
						simplex.erase(simplex.begin() + remove);
					}
					return false;
				};

				if (uab < 0.0f && vca < 0.0f) {
					if (vertexCase(0, 1, 2)) break;
				} 
				else if (ubc < 0.0f && vab < 0.0f) {
					if (vertexCase(1, 0, 2)) break;
				}
				else if (uca < 0.0f && vbc < 0.0f) {
					if (vertexCase(2, 1, 0)) break;
				}
				else if (uab >= 0.0f && vab >= 0.0f && uabc <= 0.0f) {
					if (edgeCase(uab * simplex[1] + vab * simplex[0], 2)) break;
				}
				else if (ubc >= 0.0f && vbc >= 0.0f && vabc <= 0.0f) {
					if (edgeCase(ubc * simplex[2] + vbc * simplex[1], 0)) break;
				}
				else if (uca >= 0.0f && vca >= 0.0f && wabc <= 0.0f) {
					if (edgeCase(uca * simplex[0] + vca * simplex[2], 1)) break;
				}
			}
		}
	}
	
	Vec3 color{ 1.0f };

	if (collision) {
		color = Vec3{ 0.0f, 1.0f, 0.0f };
	}
	draw(a, Vec2{ 0.0f }, color);
	draw(b, Vec2{ 0.0f }, color);

	draw(simplex, Vec2{ 0.0f }, Vec3{ 0.0f, 0.0f, 1.0f });
	Debug::drawPoint(closestPointOnSimplexToOrigin);

	// Rounding, exact computations, epsilon check.

	// TODO: Try computing the point and not vertex furthest in one direction. This will probably be worse and it might not actually converge.

	// From what is understand the mathematically collision would be always detected when the first one simplex is constructed, but due to numerical precison this wouldn't atucally work because it would need to check if a point lies directly on a line segment (the outside case is easy).  
	// From the definition of a convex polygon (non self intersecting) a line going throught it goes through at most 2 points. If I pick a random direction and get the point furthest on the polygon in that direction and then find a point furthest in the direction from that random point to the checked point then if the point is inside the polygon it has to lie on the line from the 2 points in the selected directions.

	// Circles can be treated as points.

	renderer.update(gfx);
}
