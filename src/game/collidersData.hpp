#pragma once

#include <utils/int.hpp>
#include <json/JsonValue.hpp>

#include <math/aabb.hpp>

struct MassInfo {
	float mass;
	float rotationalInertia;
};

struct CircleColliderEditor {
	float radius;
	auto displayGui() -> void;
	auto toJson() const -> Json::Value;
	static auto fromJson(const Json::Value& json) -> CircleColliderEditor;
};

struct CircleCollider : public CircleColliderEditor {
	CircleCollider(const CircleColliderEditor& circle);
	auto massInfo(float density) const -> MassInfo;
	auto aabb(Vec2 pos, float orientation) const -> Aabb;
	
};

struct BoxColliderEditor {
	Vec2 size;
	auto displayGui() -> void;
	auto toJson() const -> Json::Value;
	static auto fromJson(const Json::Value& json) -> BoxColliderEditor;
};

struct BoxCollider : public BoxColliderEditor {
	BoxCollider(const BoxColliderEditor& box);
	// @Performance: Could have an update method on a collider that would update things that are often used like the rotation matrix.
	// @Performance: Maybe store halfSize and not size because it is used more often.
	auto massInfo(float density) const -> MassInfo;
	auto aabb(Vec2 pos, float orientation) const -> Aabb;
	
};

