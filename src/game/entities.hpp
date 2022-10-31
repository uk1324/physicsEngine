#pragma once

#include <math/vec2.hpp>
#include <math/line.hpp>
#include <utils/staticVec.hpp>

#include <vector>
#include <unordered_map>

struct PhysicsMaterial {
	float bounciness;
	float staticFriction = 0.7f;
	float dynamicFriction = 1.0f;
};

enum class BodyType {
	STATIC,
	DYNAMIC,
};

struct PhysicsInfo {
	PhysicsInfo(const PhysicsMaterial* MATERIAL, float mass, BodyType bodyType, float rotationalInteria);

	Vec2 vel;
	float angularVel;
	// Should this be const? Changing the mass would also change the momentum of the system (probably doesn't matter).
	float invMass;
	float rotationalInteria;
	const PhysicsMaterial* MATERIAL;
	BodyType bodyType;
};

struct Transform {
	Vec2 pos;
	float orientation;
};

struct CircleCollider {
	float radius;
};

struct CircleEntity {
	Transform transform;
	CircleCollider collider;
	PhysicsInfo physics;
};

struct LineCollider {
	// Maybe later allow to offset the center of mass.
	float halfLength;
};

struct LineEntity {
	Transform transform;
	LineCollider collider;
	PhysicsInfo physics;
};

struct ConvexPolygonCollider {
	std::vector<Vec2> vertices;
};

struct ConvexPolygonEntity {
	Transform transform;
	ConvexPolygonCollider collider;
	PhysicsInfo physics;
};

extern std::vector<CircleEntity> circleEntites;
extern std::vector<LineEntity> lineEntites;
extern std::vector<ConvexPolygonEntity> convexPolygonEntites;

struct ContactKey {
	PhysicsInfo* aPhysics;
	Transform* aTransform;
	PhysicsInfo* bPhysics;
	Transform* bTransform;
};

struct Collision {
	Vec2 normal;
	Vec2 hitPoint;
	float penetrationDepth;
};

//struct ContactPoint {
//	ContactPoint(const Collision& collision);
//
//	Collision collision;
//};
//
//struct ContactPoint {
//	static constexpr usize MAX_CONTACT_POINTS = 1;
//	StaticVec<Collision, MAX_CONTACT_POINTS> points;
//};

//extern std::unordered_map<ContactKey, ContactPoint> contacts;