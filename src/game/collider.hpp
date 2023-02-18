#pragma once
#include <game/convexPolygonCollider.hpp>
#include <math/aabb.hpp>
#include <math/utils.hpp>
#include <math/lineSegment.hpp>

#include <math/transform.hpp>

#include <variant>

struct MassInfo {
	float mass;
	float rotationalInertia;
};

struct ConvexPolygon {
	// Could store the verts and normal into a vector of structs with edgeBegin and edgeNormal, but that would probably be confusing.

	static auto regular(i32 vertCount, float radius) -> ConvexPolygon;

	std::vector<Vec2> verts;
	// The normal i belong to the face with endpoints verts[i] and verts[(i + 1) % size].
	std::vector<Vec2> normals;

	auto calculateNormals() -> void;

	auto aabb(const Transform& transform) const->Aabb;
	auto massInfo(float density) const->MassInfo;
};

struct CircleCollider {
	float radius;
	auto massInfo(float density) const->MassInfo;
	auto aabb(const Transform& transform) const->Aabb;
};

struct BoxCollider {
	Vec2 size;

	auto massInfo(float density) const->MassInfo;
	auto aabb(const Transform& transform) const->Aabb;

	auto getCorners(const Transform& transform) const->std::array<Vec2, 4>;
	auto getEdges(const Transform& transform) const->std::array<LineSegment, 4>;
};

using Collider = std::variant<BoxCollider, CircleCollider, ConvexPolygon>;

auto massInfo(const Collider& collider, float density) -> MassInfo;
auto aabb(const Collider& collider, const Transform& transform) -> Aabb;

//using Shape = std::variant<BoxCollider, CircleCollider>;
//struct Collider {
//	Shape shape;
//
//};
