#pragma once

#include <game/editor/customGuis.hpp>
#include <game/editor/editorGuiState.hpp>
struct Commands;
struct EditorEntities;
struct Entity;
#include <utils/typeInfo.hpp>
#include <json/JsonValue.hpp>

#include <game/collidersData.hpp>
#include <variant>
#include <optional>

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

struct DistanceJointAnchorEditor {
	usize body;
	Vec2 objectSpaceOffset;

	auto editorGui(EditorGuiState& inputState, EditorEntities& entites, const Entity& entity, Commands& commands) -> void;
	auto toJson() const -> Json::Value;
	static auto fromJson(const Json::Value& json) -> DistanceJointAnchorEditor;
};

static constexpr auto DISTANCE_JOINT_ANCHOR_EDITOR_BODY_OFFSET = offsetof(DistanceJointAnchorEditor, body);
static constexpr auto DISTANCE_JOINT_ANCHOR_EDITOR_OBJECT_SPACE_OFFSET_OFFSET = offsetof(DistanceJointAnchorEditor, objectSpaceOffset);

struct DistanceJointAnchor : public DistanceJointAnchorEditor {
};

struct DistanceJointEntityEditor {
	DistanceJointAnchorEditor anchorA;
	DistanceJointAnchorEditor anchorB;
	float distance;

	auto editorGui(EditorGuiState& inputState, EditorEntities& entites, const Entity& entity, Commands& commands) -> void;
	auto toJson() const -> Json::Value;
	static auto fromJson(const Json::Value& json) -> DistanceJointEntityEditor;
};

static constexpr auto DISTANCE_JOINT_ENTITY_EDITOR_ANCHOR_A_OFFSET = offsetof(DistanceJointEntityEditor, anchorA);
static constexpr auto DISTANCE_JOINT_ENTITY_EDITOR_ANCHOR_B_OFFSET = offsetof(DistanceJointEntityEditor, anchorB);
static constexpr auto DISTANCE_JOINT_ENTITY_EDITOR_DISTANCE_OFFSET = offsetof(DistanceJointEntityEditor, distance);

struct DistanceJointEntity : public DistanceJointEntityEditor {
};

