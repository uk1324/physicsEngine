#pragma once

#include <math/vec2.hpp>
#include <math/aabb.hpp>
#include <game/entitesData.hpp>
#include <optional>
#include <variant>

struct ConvexPolygon {
	// Could store the verts and normal into a vector of structs with edgeBegin and edgeNormal, but that would probably be confusing.

	std::vector<Vec2> verts;
	// The normal i belong to the face with endpoints verts[i] and verts[(i + 1) % size].
	std::vector<Vec2> normals;

	auto calculateNormals() -> void {
		normals.clear();
		if (verts.size() < 3) {
			ASSERT_NOT_REACHED();
			return;
		}

		for (usize i = 0; i < verts.size() - 1; i++) {
			normals.push_back((verts[i + 1] - verts[i]).rotBy90deg().normalized());
		}
		normals.push_back((verts[0] - verts.back()).rotBy90deg().normalized());
	}
};

// Impulse is the change in momentum.

// Resolving velocites instead of positions directly prevents multiple constraints fighting against eachother and only the last one winning if it is impossible to fully resolve all the constraints. If all the contraints can be resolved it shouldn't be that big of an issue. At each step the positional correction would be applied once per constraint (and also the velocity correction, if it is instant, that means setting the velocity along the constrained direction to zero there may be some issues again, I think not sure).  If one constraint got resolved and another get resolved with it then everything is fine, but if one resolving breaks another one then there is an issue. You could try to resolve the constraints multiple times per step, but in many cases this probably wouldn't converge to a nice solution like the velocity constraint. Even if the velocity constraint isn't really solved the it just tries to apply a force to fix it. If positional correction were used then it might jitter back and forth depending on the order in which the constraints are resolved.

// The reason the mass is a matrix is to make it invertible.
// The jacobian seems to just be the surface normal for linear terms.
// JV + b = 0 is just a confusing way to write that the relative velocity along some vector (dot) + bias is equal 0. Sometimes certain velocites are ignored.
// Solving just calculates the magnitude of the impulse which is a change in momentum so later it is devided by the effective mass.
// The bias term is used to create new momentum.
// JV = -b
// The impluse magnitude is clamped to prevent further penetration. The value can be negative because when a collision happens the objects might already be separating.

// The below is wrong the last therm is the bias term.
// The hyper-planes visualization might be misleading because there are inifnitely many lines which correspond to a given state.
// TODO: What do transformations look like if every 3D point is represented by a 2D line. 3D points can also be converted to planes. What does this look like. Would need to choose some representation of a 3D plane. For 2D could use angle and distance from origin.

// It is the jacobian because it is literally just an array of first order partial derivatives like in the definition.

// There are way more unknows that the expressions. Each expression is a plane equation ax + by = c. Each is solved separately.

#include <math/transform.hpp>

enum class ContactPointFeature : u8 {
	FACE, VERTEX
};

struct ContactPointId {
	ContactPointFeature featureOnA;
	i16 featureOnAIndex;
	ContactPointFeature featureOnB;
	i16 featureOnBIndex;
};

struct ContactPoint {
	Vec2 position;

	// Negative if the objects are colliding. So this should always be negative, because the collide function returns nullopt if the objects are not colliding.
	float separation;

	float accumulatedNormalImpluse;
	float accumulatedTangentImpulse;	
	float invNormalEffectiveMass, invTangentEffectiveMass;

	// Solving the velocity constraint doesn't solve the position constraint. The bias term is proportional to the positional error so after iterating it the softer velocity constraint solves the position constraint.
	float bias;
	ContactPointId id;
};

struct Collision {
	auto update(const Collision& newCollision) -> void;
	auto preStep(Body& a, Body& b, float invDeltaTime) -> void;
	auto applyImpulse(Body& a, Body& b) -> void;

	// SAT with clipping can return at most 2 contactPoints. I don't think there is a case when a convex shape would need more than 2 contact points. There is either face vs face, face vs vertex or vertex vs vertex.
	ContactPoint contacts[2];
	i8 contactCount;
	// Points from body A to body B.
	Vec2 normal;

	float coefficientOfFriction;
};

auto massInfo(const Collider& collider, float density) -> MassInfo;
auto aabb(const Collider& collider, Vec2 pos, float orientation) -> Aabb;

auto collide(Vec2 aPos, float aOrientation, const Collider& aCollider, Vec2 bPos, float bOrientation, const Collider& bCollider) -> std::optional<Collision>;
auto collide(Vec2 aPos, float aOrientation, const BoxCollider& aBox, Vec2 bPos, float bOrientation, const BoxCollider& bBox) -> std::optional<Collision>;
auto collide(Vec2 boxPos, float boxOrientation, const BoxCollider& box, Vec2 circlePos, float circleOrientation, const CircleCollider& circle) -> std::optional<Collision>;
auto collide(Vec2 aPos, float aOrientation, const CircleCollider& a, Vec2 bPos, float bOrientation, const CircleCollider& b) -> std::optional<Collision>;
auto collide(const ConvexPolygon& a, const Transform& aTransform, const ConvexPolygon& b, const Transform& bTransform)->std::optional<Collision>;

auto contains(Vec2 point, Vec2 pos, float orientation, const Collider& collider) -> bool;
auto contains(Vec2 point, Vec2 pos, float orientation, const BoxCollider& box) -> bool;
auto contains(Vec2 point, Vec2 pos, float orientation, const CircleCollider& circle) -> bool;

struct RaycastResult {
	float t;
	Vec2 normal;
};

// Doesn't return a hit if the ray comes from inside the collider.
auto raycast(Vec2 rayBegin, Vec2 rayEnd, const Collider& collider, Vec2 pos, float orientation) -> std::optional<RaycastResult>;
auto raycast(Vec2 rayBegin, Vec2 rayEnd, const BoxCollider& collider, Vec2 pos, float orientation) -> std::optional<RaycastResult>;
auto raycast(Vec2 rayBegin, Vec2 rayEnd, const CircleCollider& collider, Vec2 pos, float orientation) -> std::optional<RaycastResult>;

// For intersection tests could just use collide.
auto aabbContains(const Aabb& aabb, const Collider& collider, Vec2 pos, float orientation) -> bool;