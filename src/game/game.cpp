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

// Circles can be treated as points.
static auto gjk(Span<const Vec2> aVertices, Span<const Vec2> bVertices) -> std::optional<ClosestPoints> {
	auto support = [](Vec2 dir, Span<const Vec2> vertices) -> i32 {
		ASSERT(vertices.size() != 0);
		i32 furthest{ 0 };
		auto biggestValue{ dot(vertices[furthest], dir) };
		for (i32 i = 1; i < static_cast<i32>(vertices.size()); i++) {
			// A vertex is always the furthest point in any direction on a convex polytope.
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
	ASSERT_NOT_REACHED();
}

struct Collision {
	Vec2 normal;
	Vec2 hitPoint;
	float penetrationDepth;
};

auto circleVsCircle(
	const CircleCollider& aCollider, 
	const Transform& aTransform, 
	const CircleCollider& bCollider,
	const Transform& bTransform) -> std::optional<Collision> {

	const auto distanceSquared = (aTransform.pos - bTransform.pos).lengthSq();
	if (distanceSquared < pow(aCollider.radius + bCollider.radius, 2)) {
		const auto normal = (aTransform.pos - bTransform.pos).normalized();
		return Collision{
			.normal = normal,
			.hitPoint = aTransform.pos + normal * aCollider.radius,
			.penetrationDepth = aCollider.radius + bCollider.radius - sqrt(distanceSquared)
		};
	}

	return std::nullopt;
}

auto circleVsLine(
	const CircleCollider& aCollider,
	const Transform& aTransform,
	const LineCollider& bCollider,
	const Transform& bTransform) -> std::optional<Collision> {

	const auto line = Line{ Vec2::oriented(bTransform.orientation).rotBy90deg(), 0.0 }.translated(bTransform.pos);
	const auto centerOffsetAlongLine = det(line.n, bTransform.pos);

	const auto signedDistance = ::signedDistance(line, aTransform.pos);
	if (abs(signedDistance) <= aCollider.radius) {
		const auto offsetAlongLine = det(line.n, aTransform.pos);
		// Can't just check if the offsetAlongLine is in the range because this would only work if this was an OBB oriented in the same way as the line and not a sphere.

		const auto hitNormal = (signedDistance < 0) ? line.n.normalized() : -line.n.normalized();
		const auto possibleHitPoint = aTransform.pos - hitNormal * aCollider.radius;
		if (const auto hitNonCorner = offsetAlongLine > centerOffsetAlongLine - bCollider.halfLength
			&& offsetAlongLine < centerOffsetAlongLine + bCollider.halfLength) {
			return Collision{ 
				.normal = hitNormal, 
				.hitPoint = possibleHitPoint,
				.penetrationDepth = aCollider.radius - abs(signedDistance)
			};
		}
		else {
			// minus because det computes sin and sin(pi/2 - angle) = cos(angle).
			const auto vectorAlongLine = -line.n.rotBy90deg();
			const auto corner = vectorAlongLine * std::clamp(offsetAlongLine, centerOffsetAlongLine - bCollider.halfLength, centerOffsetAlongLine + bCollider.halfLength) + line.n * line.d;
			const auto distanceFromCorner = distance(corner, aTransform.pos);
			const auto normal = (aTransform.pos - corner).normalized();
			if (const auto hitCorner = distanceFromCorner <= aCollider.radius) {
				return Collision{ 
					.normal = normal, 
					.hitPoint = corner, 					
					.penetrationDepth = aCollider.radius - (corner - aTransform.pos).length()
				};
			}
		}
	}
	return std::nullopt;
}

auto convexPolygonVsConvexPolygon(
	const ConvexPolygonCollider& aCollider,
	const Transform& aTransform,
	const ConvexPolygonCollider& bCollider,
	const Transform& bTransform) -> std::optional<Collision> {

	auto transform = [](Span<Vec2> vertices, const Transform& transform) {
		for (auto& vertex : vertices) {
			vertex *= Mat3x2::rotate(transform.orientation);
			vertex += transform.pos;
		}
	};
	auto undoTransform = [](const Vec2& vertex, const Transform& transform) {
		return (vertex - transform.pos) * Mat3x2::rotate(-transform.orientation);
	};
	auto transformVertex = [](const Vec2& vertex, const Transform& transform) {
		return vertex * Mat3x2::rotate(transform.orientation) + transform.pos;
	};
	auto aVertices = aCollider.vertices;
	transform(aVertices, aTransform);
	auto bVertices = bCollider.vertices;
	transform(bVertices, bTransform);

	if (gjk(aVertices, bVertices).has_value())
		return std::nullopt;

	auto scale = [](Span<Vec2> vertices, float scale) {
		for (auto& vertex : vertices)
			vertex *= scale;
	};

	const auto scaling = 0.75f;
	aVertices = aCollider.vertices;
	scale(aVertices, scaling);
	transform(aVertices, aTransform);
	bVertices = bCollider.vertices;
	scale(bVertices, scaling);
	transform(bVertices, bTransform);

	auto draw = [](Span<const Vec2> vertices) {
		for (usize i = 0; i < vertices.size(); i++) {
			Debug::drawLine(vertices[i], i + 1 < vertices.size() ? vertices[i + 1] : vertices[0]);
		}
	};


	const auto result = gjk(aVertices, bVertices);
	// Assume bigger penetrations can't happen. If they do maybe try using expanding polytope algorithm.
	//ASSERT(result.has_value());
	if (!result.has_value())
		return std::nullopt;

	const auto& [aClosest, bClosest] = *result;

	// All of this is just an approximation
	const auto a = undoTransform(aClosest, aTransform) / scaling;
	const auto b = undoTransform(bClosest, bTransform) / scaling;
	const auto at = transformVertex(a, aTransform);
	const auto bt = transformVertex(b, bTransform);

	//draw(aVertices);
	//draw(bVertices);
	//Debug::drawLine(at, bt);
	//Debug::drawLine(aClosest, bClosest, Vec3{ 1.0f, 0.0f, 0.0f });
	//Debug::drawPoint((aClosest + bClosest) / 2.0f);

	const auto normal{ (aClosest - bClosest).normalized() };
	auto penetrationDepth = dot(bt - at, normal);

	/*auto aV = aCollider.vertices;
	transform(aV, Transform{ aTransform.pos + normal * penetrationDepth, aTransform.orientation });
	Debug::drawLines(aV);
	auto bV = bCollider.vertices;
	transform(bV, Transform{ bTransform.pos, bTransform.orientation });
	Debug::drawLines(bV);*/

	auto aV = aCollider.vertices;
	transform(aV, aTransform);
	auto bV = bCollider.vertices;
	transform(bV, bTransform);

	Debug::drawLines(aV);
	Debug::drawLines(bV);

	// It might be better to check which features of which shape are colliding if there is a face vertex collision it is better to use the closest point on face as the contant point the collision use the scaled down shapes and the vertices of those get translated.
	//Debug::drawPoint(at + normal * penetrationDepth);
	//Debug::drawPoint(bt - normal * penetrationDepth);

	//return std::nullopt;
	return Collision{
		.normal = normal,
		.hitPoint = at + normal * penetrationDepth,
		.penetrationDepth = dot(bt - at, normal),
	};
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

auto randomNiceConvexPolygon(i32 vertexCount, float scale) -> std::vector<Vec2> {
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

	for (auto& vertex : vertices)
		vertex *= scale;

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

	//circlesScene();
	//rollingCircleScene();
	lineAndCircleScene();
}

#include <utils/io.hpp>

static auto collisionResponse(
	const Collision& collision,
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
	Would it make sense to base the time for which the friction is applied based on the coefficient of restitution of the two materials?
	The normal force is bigger the more deformed the bodies become so this would probably make the friction be longer?
	*/
	const auto ra = (collision.hitPoint - aTransform.pos).rotBy90deg();
	const auto rb = (collision.hitPoint - bTransform.pos).rotBy90deg();

	const auto coefficientOfRestitution{ (aPhysics.MATERIAL->bounciness + bPhysics.MATERIAL->bounciness) / 2.0f };
	const auto la = ra.length();
	const auto lb = rb.length();
	const Vec2 uRel{ (aPhysics.vel + aPhysics.angularVel * ra) - (bPhysics.vel + bPhysics.angularVel * rb) };

	//const auto bias{ -0.2f * (1.0f / Time::deltaTime()) * std::min(0.0f, -collision.penetrationDepth + 0.01f) };


	//auto aInteria = ((PI<float> * pow(0.2f, 4.0f)) / 4.0f);
	//auto bInteria = aInteria;
	//if (&bPhysics == &lineEntites[0].physics) {
	//	//interia = (0.005f * pow(0.6f, 3.0f)) / 12.0f;
	//	bInteria = (pow(0.6f, 2.0f) + pow(0.005f, 2.0f)) / 12.0f;
	//}

	//Debug::drawRay(aTransform.pos, ra);
	//Debug::drawRay(bTransform.pos, rb);

	/*auto k = (-coefficientOfRestitution - 1.0f) * dot(uRel, collision.normal) / (dot((1.0f * aPhysics.invMass + 1.0f * bPhysics.invMass) * collision.normal + (dot(ra, collision.normal) * aPhysics.invMass / la) * ra + (dot(rb, collision.normal) * bPhysics.invMass / lb) * rb, collision.normal));*/
	/*auto k = (-coefficientOfRestitution - 1.0f) * dot(uRel, collision.normal) / (dot((1.0f * aPhysics.invMass + 1.0f * bPhysics.invMass) * collision.normal + (dot(ra, collision.normal) * aPhysics.invMass / aPhysics.rotationalInteria * aPhysics.invMass * la) * ra + (dot(rb, collision.normal) * bPhysics.invMass * bPhysics.invMass / bPhysics.rotationalInteria * lb) * rb, collision.normal));*/

	const auto ta = (collision.hitPoint - aTransform.pos);
	const auto tb = (collision.hitPoint - bTransform.pos);
	const auto ta0 = dot(ta, collision.normal);
	const auto tb0 = dot(tb, collision.normal);

	const auto aa = (1.0f / aPhysics.rotationalInteria * (dot(ta, ta) - ta0 * ta0));
	const auto bb = (1.0f / bPhysics.rotationalInteria * (dot(tb, tb) - tb0 * tb0));

	auto k = ((-coefficientOfRestitution - 1.0f) * dot(uRel, collision.normal)) / (aPhysics.invMass + bPhysics.invMass +  aa + bb);

	k = std::max(k, 0.0f);

	//Debug::drawLine(aTransform.pos, aTransform.pos + Vec2(0.0, 1.0f) * aPhysics.angularVel * 0.001f);

	const auto parallel = collision.normal.rotBy90deg().normalized();
	// Minus to rotate by 180 deg.
	const auto aVel = -ra * aPhysics.angularVel + aPhysics.vel;
	const auto bVel = -rb * bPhysics.angularVel + bPhysics.vel;

	/*const auto normalForce = std::min(dot(aVel - bVel, collision.normal), 0.0f);*/
	const auto normalForce = std::min(dot(aVel - bVel, collision.normal), 0.0f);

	const auto coefficientOfStaticFriction{ (aPhysics.MATERIAL->staticFriction + bPhysics.MATERIAL->staticFriction) / 2.0f };
	const auto coefficientOfDynamicFriction{ (aPhysics.MATERIAL->dynamicFriction + bPhysics.MATERIAL->dynamicFriction) / 2.0f };

	auto aFriction = normalForce / aPhysics.invMass * parallel * coefficientOfDynamicFriction * ((dot(parallel, aVel) >= 0.0f) ? 1.0f : -1.0f);
	auto bFriction = normalForce / bPhysics.invMass * parallel * coefficientOfDynamicFriction * ((dot(parallel, bVel) >= 0.0f) ? 1.0f : -1.0f);

	//if (normalForce < coefficientOfStaticFriction) {
	//	/*aFriction = -dot(aVel, parallel) * parallel * (Time::deltaTime());
	//	bFriction = -dot(bVel, parallel) * parallel * Time::deltaTime();*/
	//	///*aFriction = aVel * ((dot(parallel, aVel) >= 0.0f) ? 1.0f : -1.0f);*/
	//	/*aFriction = dot(aVel / Time::deltaTime(), parallel) * parallel / aPhysics.invMass;
	//	bFriction = dot(bVel / Time::deltaTime(), parallel) * parallel / bPhysics.invMass;*/
	//	// This should be more correct but it works worse.
	//	/*aFriction = parallel * (normalForce / aPhysics.invMass) * ((dot(parallel, aVel) >= 0.0f) ? 1.0f : -1.0f);
	//	bFriction = parallel * (normalForce / bPhysics.invMass) * ((dot(parallel, bVel) >= 0.0f) ? 1.0f : -1.0f);*/
	//}

	if (abs(dot(aVel, parallel)) < 0.01f)
		aFriction = Vec2{ 0.0f };
	if (abs(dot(bVel, parallel)) < 0.01f)
		bFriction = Vec2{ 0.0f };

	// Maybe make function response that would allow reversing the normal.
	if (aPhysics.bodyType == BodyType::DYNAMIC) {
		if (bPhysics.bodyType == BodyType::STATIC)
			aTransform.pos += collision.penetrationDepth * collision.normal;

		aPhysics.vel += aFriction * aPhysics.invMass * Time::deltaTime();
		aPhysics.angularVel += det(collision.hitPoint - aTransform.pos, aFriction) * aPhysics.invMass / aPhysics.rotationalInteria * Time::deltaTime();

		aPhysics.vel += collision.normal * (k * aPhysics.invMass);
		aPhysics.angularVel -= k * dot(ra, collision.normal) * aPhysics.invMass / aPhysics.rotationalInteria;
	}
	if (bPhysics.bodyType == BodyType::DYNAMIC) {
		if (aPhysics.bodyType == BodyType::STATIC)
			bTransform.pos -= collision.penetrationDepth * collision.normal;

		bPhysics.vel += bFriction * bPhysics.invMass * Time::deltaTime();
		bPhysics.angularVel += det(collision.hitPoint - bTransform.pos, bFriction) * bPhysics.invMass / bPhysics.rotationalInteria * Time::deltaTime();

		bPhysics.vel -= collision.normal * (k * bPhysics.invMass);
		bPhysics.angularVel += k * dot(rb, collision.normal) * bPhysics.invMass / bPhysics.rotationalInteria;
	}

	Debug::drawLine(collision.hitPoint, collision.hitPoint + collision.normal * 0.f);
	if (aPhysics.bodyType == BodyType::DYNAMIC && bPhysics.bodyType == BodyType::DYNAMIC) {
		const auto seperate = ((collision.penetrationDepth / 2.0f) * collision.normal) * 1.02f;
		aTransform.pos += seperate;
		bTransform.pos -= seperate;
	}

	/*if (dot(aVel, aFriction) > 0.0f) {
		dbg(dot(aVel, aFriction));
		dbg(dot(parallel, aVel));
	}
	ASSERT(dot(aVel, aFriction) <= 0.03f);*/

	//const auto aForce = (dot(1.0f / aPhysics.invMass * aVel, parallel) * parallel) * 0.9f;
	//const auto bForce = (dot(1.0f / bPhysics.invMass * bVel, parallel) * parallel) * 0.9f;

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
}

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
	if (controlledVel != nullptr)
		(*controlledVel) += dir.normalized() * 0.5f * Time::deltaTime();

	if (Input::isKeyDown(Keycode::R)) {
		circleEntites[0].physics.angularVel += 0.2f + circleEntites[0].physics.angularVel * 2.0f;
	}

	// circle vs x
	{
		auto start{ circleEntites.begin() };
		for (auto& a : circleEntites) {
			start++;
			for (auto it = start; it != circleEntites.end(); it++) {
				auto& b{ *it };
				if (const auto collision = circleVsCircle(a.collider, a.transform, b.collider, b.transform); collision.has_value()) {
					collisionResponse(*collision, a.transform, b.transform, a.physics, b.physics);
				}
			}
			for (auto& b : lineEntites) {
				if (auto collision = circleVsLine(a.collider, a.transform, b.collider, b.transform); collision.has_value()) {
					collisionResponse(*collision, a.transform, b.transform, a.physics, b.physics);
					/*collision->normal = -collision->normal;
					collisionResponse(*collision, b.transform, a.transform, b.physics, a.physics);*/
				}
			}
		}
	}
	// convex polygon vs x
	{
		auto start{ convexPolygonEntites.begin() };
		for (auto& a : convexPolygonEntites) {
			start++;
			for (auto it = start; it != convexPolygonEntites.end(); it++) {
				auto& b{ *it };
				if (const auto collision = convexPolygonVsConvexPolygon(a.collider, a.transform, b.collider, b.transform); collision.has_value()) {
					collisionResponse(*collision, a.transform, b.transform, a.physics, b.physics);
				}
			}
		
		}
	}

	for (auto& circle : circleEntites) integrate(circle.transform, circle.physics);
	for (auto& line : lineEntites) integrate(line.transform, line.physics);
	for (auto& polygon : convexPolygonEntites) integrate(polygon.transform, polygon.physics);

	if (Input::isKeyHeld(Keycode::K)) {
		cameraZoom *= pow(3.0f, Time::deltaTime());
	}
	if (Input::isKeyHeld(Keycode::J)) {
		cameraZoom /= pow(3.0f, Time::deltaTime());
	}

	/*static auto
		a = ConvexPolygonCollider{ randomNiceConvexPolygon(6, 0.2f) },
		b = ConvexPolygonCollider{ randomNiceConvexPolygon(6, 0.2f) };

	Transform 
		aT{ .pos = Vec2{ 0.0f }, .orientation = 0.0f },
		bT{ .pos = renderer.mousePosToScreenPos(Input::cursorPos()) , .orientation = 0.0f };

	const auto c = convexPolygonVsConvexPolygon(a, aT, b, bT);*/

	if (followedPos == nullptr) {
		Vec2 dir{ 0.0f };
		if (Input::isKeyHeld(Keycode::UP))
			dir.y += 1.0f;
		if (Input::isKeyHeld(Keycode::DOWN))
			dir.y -= 1.0f;
		if (Input::isKeyHeld(Keycode::RIGHT)) 
			dir.x += 1.0f;
		if (Input::isKeyHeld(Keycode::LEFT))
			dir.x -= 1.0f;
		cameraPos += dir.normalized() * 0.5f * Time::deltaTime();

	} else {
		cameraPos = lerp(cameraPos, *followedPos, 2.0f * Time::deltaTime());
	}
		

	renderer.update(gfx, cameraPos, cameraZoom);
}

auto Game::integrate(Transform& transform, PhysicsInfo& physics) const -> void {
	if (physics.bodyType == BodyType::STATIC)
		return;

	static constexpr float groundFriction{ 0.98f };
	static constexpr float gravity = 1.0f;
	transform.pos += physics.vel * Time::deltaTime();
	/*
	Gravity is a constant acceleration because the formula for gravity is 
	F = G(m1m2/r^2)
	F = ma
	a = F/m 
	a = G(m1m2/r^2) / m1 = Gm1m2/r^2m1 = Gm2/r^2
	*/
	physics.vel.y -= Time::deltaTime() * gravityAcceleration;
	physics.vel *= pow(groundFriction, Time::deltaTime());
	physics.angularVel *= pow(groundFriction, Time::deltaTime());
	transform.orientation += physics.angularVel * Time::deltaTime();
}

/*
Useful searches: Second polar moment of area, Second moment of area

The moment of inertia of a 2D shape is obtained by integrating the distance from the center of mass squared times density with respect to area. This is because the moment of inertia of a point particle is equal to mass times the distance from the axis which it is rotating around squared.

Parallel axis theorem

Because the squared is x^2 + y^2 when integrating it can be split into two different integrals.

To calculate the moment of inertia of a shape with a center of mass (point of rotation might be more correct here because the shape's density would also need to change for the center of mass to change and the shape's shape to remain the same at the same time) translated by a vector v, dot(v, v) can be added to the moment of inertia.

Examples:

Square with sides lengths a and b and uniform density.

To calculate the distance for one axis integrate half the distance from one axis squared and multiply by two to get the integral for the whole rectangle.
integral from 0 to b/2 of D * a x^2
The density and a is constant so it can be taken out of the integral.
integral from 0 to b/2 of x^2
integral of x^2 = (x^3) / 3
((b / 2)^3) / 3 = b^3 / 8 / 3 = b^3 / 24
Then multiply by the constant 
Ix = D * a * 2 * (b^3 / 24) = D * a * b^3 / 12
Iy = D * b * a^3 / 12
Iz = Ix + Iy = (D * (a * b)(a^2 + b^2)) / 12
D = mass / area so this can also be written like this
(mass * (a^2 + b^2)) / 12

Circle with radius r

integral from 0 to r of D * 2 * pi * x * x^2 =
D * 2 * pi * integral from 0 to r of x * x^2 = 
D * 2 * pi * (r^4)/4 =
(D * pi * r^4) / 2 =
(mass * r^2) / 2
*/

static auto circleInertia(float radius, float mass) -> float {
	return mass * pow(radius, 2.0f) / 2.0f;
}

static auto rectangleInertia(float width, float height, float mass) -> float {
	return mass * pow(width, 2.0f) * pow(height, 2.0f) / 12.0f;
}

static constexpr auto DENSITY = 1.0f;
static const PhysicsMaterial MATERIAL{ .bounciness = 0.1f };

static auto makeCircle(Vec2 pos, float radius) -> CircleEntity {
	const auto mass = PI<float> * pow(radius, 2.0f) * DENSITY;
	return CircleEntity{
		.transform = { pos, 0.0f },
		.collider = { .radius = radius },
		.physics = PhysicsInfo{ &MATERIAL, mass, BodyType::DYNAMIC, circleInertia(radius, mass) }
	};
}

static auto makeLine(Vec2 pos, float halfLength) -> LineEntity {
	static const PhysicsMaterial MATERIAL{ .bounciness = 0.1f };
	const auto thickness = 0.1f;
	const auto length = halfLength * 2.0f;
	const auto mass = thickness * length * DENSITY * 50.0f; /* To small of a mass makes it spin very fast */
	return LineEntity{
		.transform = { pos, 0.0f },
		.collider = { .halfLength = halfLength },
		.physics = PhysicsInfo{ &MATERIAL, mass, BodyType::DYNAMIC, rectangleInertia(thickness, length, mass) }
	};
}


//auto Game::circlesScene() -> void {
//	for (i32 i = 0; i < 50; i++) {
//		circleEntites.push_back(CircleEntity{
//			.transform = { Vec2{ randomInRange(-0.02f, 0.02f), i * 0.12f }, 0.0f },
//			.collider = {.radius = 0.05f },
//			.physics = PhysicsInfo{ &material0, PI<float> * 0.2f * 0.2f * 20.0f, BodyType::DYNAMIC }
//		});
//	}
//
//	const auto maxMin = 0.7f;
//	const auto y = -0.5f;
//	lineEntites.push_back(LineEntity{
//		.transform = { Vec2{ 0.0f, y }, 0.0f },
//		.collider = {.halfLength = maxMin },
//		.physics = PhysicsInfo{ &material0, PI<float> * 0.3f * 0.3f * 20.0f, BodyType::STATIC },
//	});
//
//	float halfHeight = 0.4f;
//	for (const auto& sign : SIGNS) {
//		lineEntites.push_back(LineEntity{
//			.transform = { Vec2{ maxMin * sign, y + halfHeight }, PI<float> / 2.0f },
//			.collider = {.halfLength = halfHeight },
//			.physics = PhysicsInfo{ &material0, PI<float> * 0.3f * 0.3f * 20.0f, BodyType::STATIC },
//		});
//	}
//
//	controlledVel = &circleEntites[0].physics.vel;
//	gravityAcceleration = 1.0f;
//}

auto Game::rollingCircleScene() -> void {
	circleEntites.push_back(makeCircle(Vec2{ 0.4f, 0.3f }, 0.2f));

	lineEntites.push_back(LineEntity{
		.transform = { Vec2{ 0.4f, 0.1f }, 0.2f },
		.collider = { .halfLength = 0.4f },
		.physics = PhysicsInfo{ &material0, std::numeric_limits<float>::infinity(), BodyType::STATIC, std::numeric_limits<float>::infinity() },
	});

	lineEntites.push_back(LineEntity{
		.transform = { Vec2{ -0.4f, -0.4f }, -0.2f },
		/*.collider = {.halfLength = 0.8f },*/
		.collider = {.halfLength = 1.8f },
		.physics = PhysicsInfo{ &material0, std::numeric_limits<float>::infinity(), BodyType::STATIC, std::numeric_limits<float>::infinity() },
	});

	controlledVel = &circleEntites[0].physics.vel;
	gravityAcceleration = 1.0f;
}

auto Game::lineAndCircleScene() -> void {

	/*circleEntites.push_back(CircleEntity{
		.transform = { Vec2{ 0.0f, 0.0f }, 0.0f },
		.collider = {.radius = 0.2f },
		.physics = PhysicsInfo{ &material0, PI<float> * 0.2f * 0.2f * 20.0f, BodyType::DYNAMIC }
	});*/

	//circleEntites.push_back(makeCircle(Vec2{ 0.3f, 0.3f }, 0.2f ));
	lineEntites.push_back(makeLine(Vec2{ 0.0f }, 0.3f));
	/*lineEntites.push_back(LineEntity{
		.transform = { Vec2{ 0.4f, 0.3f }, -0.2f },
		.collider = { .halfLength = 0.3f },
		.physics = PhysicsInfo{ &material0, PI<float> * 0.1f * 0.1f * 20.0f, BodyType::DYNAMIC },
	});*/

	std::uniform_real_distribution random(0.0f, 0.5f);
	std::default_random_engine engine;
	for (i32 i = 0; i < 20; i++) {
		circleEntites.push_back(makeCircle(Vec2{ random(engine), random(engine) }, 0.07f));
		//circleEntites.push_back(CircleEntity{
		//	.transform = { Vec2{ random(engine), random(engine) }, 0.0f },
		//	.collider = {.radius = 0.07f },
		//	.physics = PhysicsInfo{ &material0, PI<float> * 0.2f * 0.2f * 20.0f, BodyType::DYNAMIC }
		//});
	}

	gravityAcceleration = 0.0f;
	controlledVel = &lineEntites[0].physics.vel; 
	followedPos = &lineEntites[0].transform.pos;
}
//
//auto Game::twoConvexPolygonsScene() -> void {
//
//	convexPolygonEntites.push_back(ConvexPolygonEntity{
//		.transform = { Vec2{ 0.4f, 0.3f }, -0.2f },
//		.collider = { randomNiceConvexPolygon(5, 0.2f) },
//		.physics = PhysicsInfo{ &material0, PI<float> * 0.1f * 0.1f * 20.0f, BodyType::DYNAMIC },
//	});
//
//	convexPolygonEntites.push_back(ConvexPolygonEntity{
//		.transform = { Vec2{ 0.0f, 0.0f }, -0.2f },
//		.collider = { randomNiceConvexPolygon(6, 0.2f) },
//		.physics = PhysicsInfo{ &material0, PI<float> * 0.2f * 0.2f * 20.0f, BodyType::DYNAMIC },
//	});
//
//	convexPolygonEntites.push_back(ConvexPolygonEntity{
//		.transform = { Vec2{ -0.2f, -0.33f }, -0.2f },
//		.collider = { randomNiceConvexPolygon(3, 0.2f) },
//		.physics = PhysicsInfo{ &material0, PI<float> * 0.2f * 0.2f * 20.0f, BodyType::DYNAMIC },
//	});
//
//	convexPolygonEntites.push_back(ConvexPolygonEntity{
//		.transform = { Vec2{ -0.2f, 0.43f }, -0.2f },
//		.collider = { randomNiceConvexPolygon(8, 0.2f) },
//		.physics = PhysicsInfo{ &material0, PI<float> * 0.2f * 0.2f * 20.0f, BodyType::DYNAMIC },
//	});
//
//
//	//for (i32 i = 0; i < 20; i++) {
//	//	convexPolygonEntites.push_back(ConvexPolygonEntity{
//	//		.transform = { Vec2{ randomInRange(-2.0f, 2.0f), randomInRange(-2.0f, 2.0f) }, -0.0f },
//	//		.collider = { randomNiceConvexPolygon(i32(round(randomInRange(3, 7))), 0.1f) },
//	//		.physics = PhysicsInfo{ &material0, PI<float> * 0.2f * 0.2f * 20.0f, BodyType::DYNAMIC },
//	//	});
//	//}
//
//	gravityAcceleration = 0.0f;
//	controlledVel = &convexPolygonEntites[0].physics.vel;
//}
//
