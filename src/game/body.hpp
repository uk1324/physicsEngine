//#pragma once
//
//#include <math/transform.hpp>
//
//struct BodyOldEditor {
//	Transform transform;
//	Vec2 vel;
//	float angularVel;
//	float mass;
//	float rotationalInertia;
//	float coefficientOfFriction;
//	Collider collider;
//
//	auto editorGui(EditorGuiState& inputState, EditorEntities& entites, const Entity& entity, Commands& commands) -> void;
//	auto toJson() const->Json::Value;
//	static auto fromJson(const Json::Value& json)->BodyOldEditor;
//};