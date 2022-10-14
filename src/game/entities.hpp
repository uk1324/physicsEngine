#pragma once

#include <math/vec2.hpp>
#include <math/line.hpp>

#include <vector>

struct PhysicsMaterial {
	float bounciness;
};

struct PhysicsInfo {
	PhysicsInfo(const PhysicsMaterial* material, float mass);

	Vec2 vel;
	float angularVel;
	// Should this be const? Changing the mass would also change the momentum of the system (probably doesn't matter).
	float invMass;
	const PhysicsMaterial* material;
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
extern std::vector<ConvexPolygonCollider> convexPolygonEntites;