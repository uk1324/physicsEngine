#pragma once

#include <math/vec2.hpp>
#include <math/aabb.hpp>
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
//
struct ContactPoint
{
	ContactPoint()
		: Pn(0.0f)
		, Pt(0.0f)
		, Pnb(0.0f) {
		penetrationDepth = 0.0f;
		massNormal = 0.0f;
		bias = 0.0f;
		massTangent = 0.0f;
		feature = { 0 };
	}

	Vec2 position;
	// From body1 to body2
	Vec2 normal;
	Vec2 r1, r2;

	// Seperating distance along normal. Can be negative.
	float penetrationDepth;

	float Pn;	// accumulated normal impulse
	float Pt;	// accumulated tangent impulse
	float Pnb;	// accumulated normal impulse for position bias
	float massNormal, massTangent;

	// Solving the velocity constraint doesn't solve the position constraint. The bias term is proportional to the positional error so after iterating it the softer velocity constraint solves the position constraint.
	float bias;
	FeaturePair feature;
};

struct Body;

struct Collision {
	Collision() 
		: contactCount{ 0 }
		, coefficientOfFriction{ 0.0f } {}

	auto update(ContactPoint* contacts, int numContacts) -> void;

	auto preStep(Body* a, Body* b, float inv_dt) -> void;
	auto applyImpulse(Body* a, Body* b) -> void;

	static constexpr i32 MAX_CONTACTS = 2;
	ContactPoint contacts[MAX_CONTACTS];

	i32 contactCount;
	float coefficientOfFriction;
};

struct MassInfo {
	float mass;
	float rotationalInertia;
};

// TODO: Allow translating the moment of inertia.
struct BoxCollider {
	Vec2 size;

	// @Performance: Could have an update method on a collider that would update things that are often used like the rotation matrix.
	// @Performance: Maybe store halfSize and not size because it is used more often.
	auto massInfo(float density) const -> MassInfo;
	auto aabb(Vec2 pos, float orientation) const -> Aabb;
};

struct CircleCollider {
	float radius;

	auto massInfo(float density) const -> MassInfo;
	auto aabb(Vec2 pos, float orientation) const -> Aabb;
};

using Collider = std::variant<BoxCollider, CircleCollider>;

auto massInfo(const Collider& collider, float density) -> MassInfo;
auto aabb(const Collider& collider, Vec2 pos, float orientation) -> Aabb;

auto collide(Vec2 aPos, float aOrientation, const Collider& aCollider, Vec2 bPos, float bOrientation, const Collider& bCollider) -> std::optional<Collision>;
auto contains(Vec2 point, Vec2 pos, float orientation, const Collider& collider) -> bool;

auto collide(Vec2 aPos, float aOrientation, const BoxCollider& aBox, Vec2 bPos, float bOrientation, const BoxCollider& bBox) -> std::optional<Collision>;
auto collide(Vec2 boxPos, float boxOrientation, const BoxCollider& box, Vec2 circlePos, float circleOrientation, const CircleCollider& circle) -> std::optional<Collision>;
auto collide(Vec2 aPos, float aOrientation, const CircleCollider& a, Vec2 bPos, float bOrientation, const CircleCollider& b)->std::optional<Collision>;


auto contains(Vec2 point, Vec2 pos, float orientation, const BoxCollider& box) -> bool;
auto contains(Vec2 point, Vec2 pos, float orientation, const CircleCollider& circle) -> bool;

struct RaycastResult {
	float t;
	Vec2 normal;
};

// Doesn't return a hit if the ray comes from inside the collider.
auto raycast(Vec2 rayBegin, Vec2 rayEnd, const Collider& collider, Vec2 pos, float orientation) -> std::optional<RaycastResult>;
auto raycast(Vec2 rayBegin, Vec2 rayEnd, const BoxCollider& collider, Vec2 pos, float orientation) -> std::optional<RaycastResult>;
auto raycast(Vec2 rayBegin, Vec2 rayEnd, const CircleCollider& collider, Vec2 pos, float orientation)->std::optional<RaycastResult>;