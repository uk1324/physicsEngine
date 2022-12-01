#pragma once

#include <utils/int.hpp>
#include <json/JsonValue.hpp>

#include <math/aabb.hpp>

struct MassInfo {
	float mass;
	float rotationalInertia;
};

struct CircleCollider {
	float radius;
	auto massInfo(float density) const -> MassInfo;
	auto aabb(Vec2 pos, float orientation) const -> Aabb;
	
	auto displayGui() -> void;
	auto toJson() const -> Json::Value;
	static auto fromJson(const Json::Value& json) -> CircleCollider;
};

struct BoxCollider {
	Vec2 size;
	// @Performance: Could have an update method on a collider that would update things that are often used like the rotation matrix.
	// @Performance: Maybe store halfSize and not size because it is used more often.
	auto massInfo(float density) const -> MassInfo;
	auto aabb(Vec2 pos, float orientation) const -> Aabb;
	
	auto displayGui() -> void;
	auto toJson() const -> Json::Value;
	static auto fromJson(const Json::Value& json) -> BoxCollider;
};

