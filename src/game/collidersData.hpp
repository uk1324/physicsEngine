#pragma once

#include <game/editor/editorGuiState.hpp>
struct Commands;
struct EditorEntities;
struct Entity;
#include <utils/typeInfo.hpp>
#include <json/JsonValue.hpp>

#include <math/aabb.hpp>

struct MassInfo {
	float mass;
	float rotationalInertia;
};

struct CircleColliderEditor {
	float radius;

	auto editorGui(EditorGuiState& inputState, EditorEntities& entites, const Entity& entity, Commands& commands) -> void;
	auto toJson() const -> Json::Value;
	static auto fromJson(const Json::Value& json) -> CircleColliderEditor;
};

static constexpr auto CIRCLE_COLLIDER_EDITOR_RADIUS_OFFSET = offsetof(CircleColliderEditor, radius);

struct CircleCollider : public CircleColliderEditor {
	CircleCollider(const CircleColliderEditor& circle);
	auto massInfo(float density) const -> MassInfo;
	auto aabb(Vec2 pos, float orientation) const -> Aabb;
	
};

struct BoxColliderEditor {
	Vec2 size;

	auto editorGui(EditorGuiState& inputState, EditorEntities& entites, const Entity& entity, Commands& commands) -> void;
	auto toJson() const -> Json::Value;
	static auto fromJson(const Json::Value& json) -> BoxColliderEditor;
};

static constexpr auto BOX_COLLIDER_EDITOR_SIZE_OFFSET = offsetof(BoxColliderEditor, size);

struct BoxCollider : public BoxColliderEditor {
	BoxCollider(const BoxColliderEditor& box);
	// @Performance: Could have an update method on a collider that would update things that are often used like the rotation matrix.
	// @Performance: Maybe store halfSize and not size because it is used more often.
	auto massInfo(float density) const -> MassInfo;
	auto aabb(Vec2 pos, float orientation) const -> Aabb;
	
};

