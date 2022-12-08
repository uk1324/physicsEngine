#pragma once

#include <utils/int.hpp>
#include <json/JsonValue.hpp>

#include <game/collidersData.hpp>
#include <variant>

using Collider = std::variant<BoxCollider, CircleCollider>;
auto colliderToJson(const Collider& collider) -> Json::Value;
auto jsonToCollider(const Json::Value& json) -> Collider;
auto displayColliderGui(const Collider& collider) -> void;

struct BodyEditor {
	Vec2 pos;
	float orientation;
	Vec2 vel;
	float angularVel;
	float mass;
	float rotationalInertia;
	float coefficientOfFriction;
	Collider collider;
	auto displayGui() -> void;
	auto toJson() const -> Json::Value;
	static auto fromJson(const Json::Value& json) -> BodyEditor;
};

struct Body : public BodyEditor {
	Body(Vec2 pos, const Collider& collider, bool isStatic);
	Body(const BodyEditor& body);
	auto updateInvMassAndInertia() -> void;
	Body();
	auto isStatic() const -> bool { return invMass == 0.0f; }

	Vec2 force;
	float torque;
	float invMass;
	float invRotationalInertia;
	
};

