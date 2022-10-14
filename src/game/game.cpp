#include <game/game.hpp>
#include <game/input.hpp>
#include <engine/time.hpp>
#include <engine/window.hpp>
#include <game/debug.hpp>
#include <math/utils.hpp>
#include <math/mat2.hpp>
#include <utils/random.hpp>
#include <utils/span.hpp>

#include <optional>
#include <random>

// For the physics to run correctly it has to be updated always at the same rate when the time scale is halved for the game to run smoothly the engine should save 2 physics game states and interpolate between them. If time scale is doubled the physics has to run at double the rate. The whole game should run at twice the rate for it to be correct.

/*
Checking collision between polygons could be done by checking if the new position intersects.
if intersects(pos + vel) {
	normal, hitPoint = gjk(pos)
}
*/

// When using polling you have to save all the state and then the user can decide what information they actually need to use. When using callbacks the user can store the state they need and use it later.

// Another idea I had is to generate random points in square or some other shape untill a n sided convex hull can be made from it.

// For some cases it might be better to not transform all the shape's points and only do it in the support function and also rotate the support direction.

struct ClosestPoints {
	Vec2 closestPointOnA;
	Vec2 closestPointOnB;
};

// When using SAT the separating axis can be cached between frames. At tested as the first axis on the next frame

static auto gjk(Span<const Vec2> aVertices, Span<const Vec2> bVertices) -> std::optional<ClosestPoints> {
	auto support = [](Vec2 dir, Span<const Vec2> vertices) -> i32 {
		ASSERT(vertices.size() != 0);
		i32 furthest{ 0 };
		auto biggestValue{ dot(vertices[furthest], dir) };
		for (i32 i = 1; i < static_cast<i32>(vertices.size()); i++) {
			if (const auto value = dot(vertices[i], dir); value > biggestValue) {
				biggestValue = value;
				furthest = i;
			}
		}
		return furthest;
	};

	struct SimplexVertex {
		i32 aIndex;
		i32 bIndex;
		Vec2 pos;

		auto operator==(const SimplexVertex& other) const -> bool {
			return pos == other.pos;
		};
	};

	float simplexU, simplexV;
	auto minkowskiDifferenceSupport = [&](Vec2 dir, Span<const Vec2> a, Span<const Vec2> b) -> SimplexVertex {
		const auto aIndex = support(dir, aVertices);
		const auto bIndex = support(-dir, bVertices);
		return SimplexVertex{
			aIndex,
			bIndex,
			// Try without minus at end becuase it shouldn't be there it is there to return the direction from the origin to the actual support.
			-(a[aIndex] - b[bIndex])
		};
	};

	Vec2 direction{ 1.0f, 0.0f };

	std::vector<SimplexVertex> simplex{ minkowskiDifferenceSupport(direction, aVertices, bVertices) };

	bool collision = false;
	Vec2 closestPointOnSimplexToOrigin;
	int count = 0;
	SimplexVertex p0;
	SimplexVertex p1;
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
				closestPointOnSimplexToOrigin = simplex[0].pos;
				const auto newSupport = minkowskiDifferenceSupport(closestPointOnSimplexToOrigin, aVertices, bVertices);
				if (simplex[0] == newSupport) {
					count = 1;
					p0 = simplex[0];
					break;
				}
				simplex.push_back(newSupport);
			}
			else if (simplex.size() == 2) {
				const auto [u, v] = lineBarycentric(simplex[0].pos, simplex[1].pos);
				if (u < 0.0f) {
					closestPointOnSimplexToOrigin = simplex[0].pos;
					const auto newSupport = minkowskiDifferenceSupport(closestPointOnSimplexToOrigin, aVertices, bVertices);
					if (std::find(simplex.begin(), simplex.end(), newSupport) != simplex.end()) {
						count = 1;
						p0 = simplex[0];
						break;
					}
					else {
						simplex.erase(simplex.begin() + 1);
					}
				}
				else if (v < 0.0f) {
					closestPointOnSimplexToOrigin = simplex[1].pos;
					const auto newSupport = minkowskiDifferenceSupport(closestPointOnSimplexToOrigin, aVertices, bVertices);
					if (std::find(simplex.begin(), simplex.end(), newSupport) != simplex.end()) {
						count = 1;
						p0 = simplex[1];
						break;
					}
					else {
						simplex.erase(simplex.begin() + 0);
					}
				}
				else {
					closestPointOnSimplexToOrigin = u * simplex[1].pos + v * simplex[0].pos;
					const auto newSupport = minkowskiDifferenceSupport(closestPointOnSimplexToOrigin, aVertices, bVertices);
					if (std::find(simplex.begin(), simplex.end(), newSupport) != simplex.end()) {
						count = 2;
						p0 = simplex[0];
						p1 = simplex[1];
						simplexU = u;
						simplexV = v;
						break;
					}
					else {
						simplex.push_back(newSupport);
					}
				};
			}
			else {
				const auto
					e0 = simplex[1].pos - simplex[0].pos,
					e1 = simplex[2].pos - simplex[1].pos,
					e2 = simplex[0].pos - simplex[2].pos;
				const auto
					l0 = /* Vec2{ 0.0f } */ -simplex[0].pos,
					l1 = /* Vec2{ 0.0f } */ -simplex[1].pos,
					l2 = /* Vec2{ 0.0f } */ -simplex[2].pos;
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

				const auto [uab, vab] = lineBarycentric(simplex[0].pos, simplex[1].pos);
				const auto [ubc, vbc] = lineBarycentric(simplex[1].pos, simplex[2].pos);
				const auto [uca, vca] = lineBarycentric(simplex[2].pos, simplex[0].pos);

				auto vertexCase = [&](usize pointIndex, usize remove1, usize remove2) {
					closestPointOnSimplexToOrigin = simplex[pointIndex].pos;
					const auto newSupport = minkowskiDifferenceSupport(closestPointOnSimplexToOrigin, aVertices, bVertices);
					if (std::find(simplex.begin(), simplex.end(), newSupport) != simplex.end()) {
						count = 1;
						p0 = simplex[pointIndex];
						return true;
					}
					else {
						simplex.erase(simplex.begin() + remove1);
						simplex.erase(simplex.begin() + remove2);
					}
					return false;
				};

				auto edgeCase = [&](Vec2 point, usize remove) {
					closestPointOnSimplexToOrigin = point;
					const auto newSupport = minkowskiDifferenceSupport(closestPointOnSimplexToOrigin, aVertices, bVertices);
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
					if (edgeCase(uab * simplex[1].pos + vab * simplex[0].pos, 2)) {
						count = 2;
						p0 = simplex[0];
						p1 = simplex[1];
						simplexU = uab;
						simplexV = vab;
						break;
					}
				}
				else if (ubc >= 0.0f && vbc >= 0.0f && vabc <= 0.0f) {
					if (edgeCase(ubc * simplex[2].pos + vbc * simplex[1].pos, 0)) {
						count = 2;
						p0 = simplex[1];
						p1 = simplex[2];
						simplexU = ubc;
						simplexV = vbc;
						break;
					}
				}
				else if (uca >= 0.0f && vca >= 0.0f && wabc <= 0.0f) {
					if (edgeCase(uca * simplex[0].pos + vca * simplex[2].pos, 1)) {
						count = 2;
						p0 = simplex[2];
						p1 = simplex[0];
						simplexU = uca;
						simplexV = vca;
						break;
					}
				}
			}
		}
	}

	if (collision) {
		return std::nullopt;
	}

	Vec2 closestA;
	Vec2 closestB;

	if (count == 1) {
		return ClosestPoints{ .closestPointOnA = aVertices[p0.aIndex], .closestPointOnB = bVertices[p0.bIndex] };
	}
	else if (count == 2) {
		return ClosestPoints{ 
			.closestPointOnA = simplexU * aVertices[p1.aIndex] + simplexV * aVertices[p0.aIndex],
			.closestPointOnB = simplexU * bVertices[p1.bIndex] + simplexV * bVertices[p0.bIndex]
		};
	}
}

struct Collision {
	Vec2 normal;
	Vec2 hitPoint;
};

static auto convexPolygonVsConvexPolygon(Span<const Vec2> aVertices, Span<const Vec2> bVertices) -> std::optional<Collision> {

}

static auto randomConvexPolygon(i32 vertexCount) -> std::vector<Vec2> {
	ASSERT(vertexCount >= 3);
	const auto triangleCount = vertexCount - 2; // Every convex n-gon can be divided into n - 2 triangles	
	const auto fanAngle = randomInRange(TAU<float> / 6.0f, PI<float> -0.1f);

	std::uniform_real_distribution<float> generateLength(0.2f, 0.3f);
	std::vector<Vec2> vertices;
	vertices.push_back(Vec2{ 0.0f } /* Shared vertex */);
	auto currentAngle = randomInRange(0.0f, TAU<float>);
	vertices.push_back(Vec2::oriented(currentAngle) * randomInRange(0.2f, 0.3f));
	auto v0 = vertices.back().rotBy90deg();
	for (i32 i = 0; i < triangleCount; i++) {
		// This just makes all the fan angles equal. Random numbers whose sum is equal to some number can be generated using a divide and conquer algorithm, but making them equal makes the shapes nicer because the vertices have to be separated from eachother.
		const auto angle = fanAngle / triangleCount;
		currentAngle += angle;

		const auto v1 = Vec2::oriented(currentAngle);
		const auto& p0 = vertices.back();
		const auto& p1 = Vec2{ 0.0f };
		/*
		p0 + a * v0 = p1 + b * v1
		a * v0 - b * v1 = p1 - p0
		[a, b] * mat2(v0, -v1) = p1 - p0
		*/
		// Solve using Cramer's rule.
		const auto x{ p1 - p0 };
		const auto mDet{ Mat2{ v0, -v1 }.det() };
		ASSERT(mDet != 0.0f);
		const auto a = Mat2{ x, -v1 }.det() / mDet;
		const auto intersection = p0 + a * v0;
		const auto maxLength = intersection.length();
		vertices.push_back(v1 * randomInRange(std::min(maxLength / 2.0f, 0.3f), std::min(maxLength, 0.5f)));
		v0 = vertices.back() - vertices[vertices.size() - 2];
	}

	Vec2 centerOfMass{ 0.0f };
	for (const auto& vertex : vertices) {
		centerOfMass += vertex;
	}
	centerOfMass /= static_cast<float>(vertices.size());

	/*auto maxDistanceFromCenter = 0.0f;
	for (auto& vertex : vertices) {
		vertex -= centerOfMass;
		const auto distanceFromCenter = vertex.length();
		if (vertex.length() > maxDistanceFromCenter) {
			maxDistanceFromCenter = distanceFromCenter;
		}
	}*/

	auto averageDistanceToCenter = 0.0f;
	for (auto& vertex : vertices) {
		vertex -= centerOfMass;
		const auto distanceToCenter = vertex.length();
		averageDistanceToCenter += distanceToCenter;
	}
	averageDistanceToCenter /= vertices.size();

	for (auto& vertex : vertices) {
		vertex /= averageDistanceToCenter;
	}

	return vertices;
}

auto randomNiceConvexPolygon(i32 vertexCount) -> std::vector<Vec2> {
	auto vertices = randomConvexPolygon(vertexCount);
	for (;;) {
		auto minDistanceToCenter = std::numeric_limits<float>::infinity();
		auto maxDistanceToCenter = 0.0f;
		for (const auto& vertex : vertices) {
			const auto distanceToCenter = vertex.length();
			if (distanceToCenter < minDistanceToCenter)
				minDistanceToCenter = distanceToCenter;

			if (distanceToCenter > maxDistanceToCenter)
				maxDistanceToCenter = distanceToCenter;
		}

		if (abs(minDistanceToCenter - maxDistanceToCenter) > 0.6f) {
			vertices = randomConvexPolygon(vertexCount);
		} else {
			break;
		}
	}
	return vertices;
}


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

	/*
	Would it make sense to base the time the friction is applied based on the coefficient of restitution of the two materials?
	The normal force is bigger the more deformed the bodies become so this would probably make the friction be longer?
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

	auto draw = [this](const std::vector<Vec2>& vertices, Vec3 color) {
		for (size_t i = 0; i < vertices.size(); i++) {
			Debug::drawLine((vertices[i]), i + 1 < vertices.size() ? vertices[i + 1] : vertices[0], color);
		}
	};

	std::vector<Vec2> a{ Vec2{ -0.2f, -0.3f }, Vec2{ -0.2f, 0.1f }, Vec2{ 0.01f, 0.2f }, Vec2{ 0.2f, -0.1f } };
	Vec2 aPos{ renderer.mousePosToScreenPos(Input::cursorPos()) };
	for (auto& v : a) {
		v += aPos;
	}
	const std::vector<Vec2> b{ Vec2{ -0.2f, -0.3f }, Vec2{ -0.2f, 0.1f }, Vec2{ 0.01f, 0.2f }, Vec2{ 0.2f, -0.1f } };
	Vec2 bPos{ 0.0f };


	const auto result = gjk(std::as_const(a), b);
	if (result.has_value()) {
		Debug::drawLine(result->closestPointOnA, result->closestPointOnB);
	}
	draw(a, Vec3{ 1.0f });
	draw(b, Vec3{ 1.0f });

	// Rounding, exact computations, epsilon check.

	// TODO: Try computing the point and not vertex furthest in one direction. This will probably be worse and it might not actually converge.

	// From what is understand the mathematically collision would be always detected when the first one simplex is constructed, but due to numerical precison this wouldn't atucally work because it would need to check if a point lies directly on a line segment (the outside case is easy).  
	// From the definition of a convex polygon (non self intersecting) a line going throught it goes through at most 2 points. If I pick a random direction and get the point furthest on the polygon in that direction and then find a point furthest in the direction from that random point to the checked point then if the point is inside the polygon it has to lie on the line from the 2 points in the selected directions.

	// Circles can be treated as points.

	renderer.update(gfx);
}
