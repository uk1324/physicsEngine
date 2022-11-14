#pragma once

#include <math/vec2.hpp>
#include <math/aabb.hpp>
#include <optional>
#include <variant>

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

struct ContactPoint
{
	ContactPoint()
		: Pn(0.0f)
		, Pt(0.0f)
		, Pnb(0.0f) {
		separation = 0.0f;
		massNormal = 0.0f;
		bias = 0.0f;
		massTangent = 0.0f;
		feature = { 0 };
	}

	Vec2 position;
	Vec2 normal;
	Vec2 r1, r2;
	float separation;
	float Pn;	// accumulated normal impulse
	float Pt;	// accumulated tangent impulse
	float Pnb;	// accumulated normal impulse for position bias
	float massNormal, massTangent;
	float bias;
	FeaturePair feature;
};

struct Body;

struct Collision {
	Collision() {
		coefficientOfFriction = 0.0f;
		numContacts = 0;
	};

	void Update(ContactPoint* contacts, int numContacts);

	void PreStep(Body* a, Body* b, float inv_dt);
	void ApplyImpulse(Body* a, Body* b);

	static constexpr i32 MAX_CONTACTS = 2;
	ContactPoint contacts[MAX_CONTACTS];

	i32 numContacts;

	// Combined friction
	float coefficientOfFriction;
};

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