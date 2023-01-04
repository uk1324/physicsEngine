#pragma once

#include <math/vec2.hpp>
#include <math/aabb.hpp>
#include <game/entitesData.hpp>
#include <optional>
#include <variant>

union FeaturePair
{
	struct Edges
	{
		char inEdge1;
		char outEdge1;
		char inEdge2;
		char outEdge2;
	} e;
	int value;
};


// Impulse is the change in momentum.

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
struct ContactPoint {
	ContactPoint()
		: accumulatedNormalImpluse{ 0.0f }
		, accumulatedTangentImpulse{ 0.0f }
		, penetrationDepth{ 0.0f }
		, invNormalEffectiveMass{ 0.0f }
		, bias{ 0.0f }
		, invTangentEffectiveMass{ 0.0f }
		, feature{ 0 } {}

	Vec2 position;
	// From body1 to body2
	Vec2 normal;

	// Seperating distance along normal. Can be negative.
	float penetrationDepth;

	float accumulatedNormalImpluse;
	float accumulatedTangentImpulse;	
	float invNormalEffectiveMass, invTangentEffectiveMass;

	// Solving the velocity constraint doesn't solve the position constraint. The bias term is proportional to the positional error so after iterating it the softer velocity constraint solves the position constraint.
	float bias;
	FeaturePair feature;
};

struct Collision {
	Collision() 
		: contactCount{ 0 }
		, coefficientOfFriction{ 0.0f } {}

	auto update(ContactPoint* contacts, i32 numContacts) -> void;

	auto preStep(Body* a, Body* b, float invDeltaTime) -> void;
	auto applyImpulse(Body& a, Body& b) -> void;

	static constexpr i32 MAX_CONTACT_COUNT = 2;
	ContactPoint contacts[MAX_CONTACT_COUNT];
	i32 contactCount;

	float coefficientOfFriction;
};

auto massInfo(const Collider& collider, float density) -> MassInfo;
auto aabb(const Collider& collider, Vec2 pos, float orientation) -> Aabb;

auto collide(Vec2 aPos, float aOrientation, const Collider& aCollider, Vec2 bPos, float bOrientation, const Collider& bCollider) -> std::optional<Collision>;
auto contains(Vec2 point, Vec2 pos, float orientation, const Collider& collider) -> bool;

auto collide(Vec2 aPos, float aOrientation, const BoxCollider& aBox, Vec2 bPos, float bOrientation, const BoxCollider& bBox) -> std::optional<Collision>;
auto collide(Vec2 boxPos, float boxOrientation, const BoxCollider& box, Vec2 circlePos, float circleOrientation, const CircleCollider& circle) -> std::optional<Collision>;
auto collide(Vec2 aPos, float aOrientation, const CircleCollider& a, Vec2 bPos, float bOrientation, const CircleCollider& b) -> std::optional<Collision>;

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