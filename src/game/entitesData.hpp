#pragma once

#include <game/editor/editorGuiState.hpp>
struct Commands;
struct EditorEntities;
struct Entity;
#include <utils/typeInfo.hpp>
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

	auto editorGui(EditorGuiState& inputState, EditorEntities& entites, const Entity& entity, Commands& commands) -> void;
	auto toJson() const -> Json::Value;
	static auto fromJson(const Json::Value& json) -> BodyEditor;
};

static constexpr auto BODY_EDITOR_POS_OFFSET = offsetof(BodyEditor, pos);
static constexpr auto BODY_EDITOR_ORIENTATION_OFFSET = offsetof(BodyEditor, orientation);
static constexpr auto BODY_EDITOR_VEL_OFFSET = offsetof(BodyEditor, vel);
static constexpr auto BODY_EDITOR_ANGULAR_VEL_OFFSET = offsetof(BodyEditor, angularVel);
static constexpr auto BODY_EDITOR_MASS_OFFSET = offsetof(BodyEditor, mass);
static constexpr auto BODY_EDITOR_ROTATIONAL_INERTIA_OFFSET = offsetof(BodyEditor, rotationalInertia);
static constexpr auto BODY_EDITOR_COEFFICIENT_OF_FRICTION_OFFSET = offsetof(BodyEditor, coefficientOfFriction);
static constexpr auto BODY_EDITOR_COLLIDER_OFFSET = offsetof(BodyEditor, collider);

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

