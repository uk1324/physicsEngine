#pragma once

#include <utils/int.hpp>
#include <json/JsonValue.hpp>

#include <game/collidersData.hpp>
#include <variant>

using Collider = std::variant<BoxCollider, CircleCollider>;
auto colliderToJson(const Collider& collider) -> Json::Value;
auto jsonToCollider(const Json::Value& json) -> Collider;
auto displayColliderGui(const Collider& collider) -> void;

struct Body {
	Vec2 pos;
	float orientation;
	float coefficientOfFriction;
	float mass;
	float rotationalInertia;
	Collider collider;
	Body(Vec2 pos, const Collider& collider, bool isStatic);
	Body() {}
	auto isStatic() const -> bool { return invMass == 0.0f; }

	Vec2 vel;
	Vec2 force;

	float angularVel;
	float torque;
	float invMass;
	float invRotationalInertia;
	
	auto displayGui() -> void;
	auto toJson() const -> Json::Value;
	static auto fromJson(const Json::Value& json) -> Body;
};

