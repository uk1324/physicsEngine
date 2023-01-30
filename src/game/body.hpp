#pragma once

#include <game/collidersData.hpp>
#include <variant>
#include <optional>
#include <game/entitesData.hpp>
#include <math/transform.hpp>
#include <game/entityArray.hpp>

using Collider = std::variant<BoxCollider, CircleCollider>;

struct Body {
	Body();
	Body(Vec2 pos, const Collider& collider, bool isStatic);
	Body(const BodyOldEditor& body);
	auto updateInvMassAndInertia() -> void;
	auto isStatic() const -> bool;

	Transform transform;
	Vec2 vel;
	float angularVel;
	float mass;
	float rotationalInertia;
	float coefficientOfFriction;
	Collider collider;
	Vec2 force;
	float torque;
	float invMass;
	float invRotationalInertia;

	/*auto editorGui(EditorGuiState& inputState, EditorEntities& entites, const Entity& entity, Commands& commands) -> void;
	auto toJson() const->Json::Value;
	static auto fromJson(const Json::Value& json)->BodyEditor;*/
};

using BodyId = EntityArray<Body>::Id;
