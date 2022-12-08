#include <game\entitesData.hpp>
#include <imgui/imgui.h>

using namespace ImGui;
using namespace Json;

auto BodyEditor::displayGui() -> void {
	InputFloat2("pos", pos.data());
	InputFloat("orientation", &orientation);
	InputFloat2("vel", vel.data());
	InputFloat("angularVel", &angularVel);
	InputFloat("mass", &mass);
	InputFloat("rotationalInertia", &rotationalInertia);
	InputFloat("coefficientOfFriction", &coefficientOfFriction);
	displayColliderGui(collider);
}

auto BodyEditor::toJson() const -> Json::Value {
	auto result = Json::Value::emptyObject();
	result["pos"] = { { "x", pos.x }, { "y", pos.y } };
	result["orientation"] = Json::Value(orientation);
	result["vel"] = { { "x", vel.x }, { "y", vel.y } };
	result["angularVel"] = Json::Value(angularVel);
	result["mass"] = Json::Value(mass);
	result["rotationalInertia"] = Json::Value(rotationalInertia);
	result["coefficientOfFriction"] = Json::Value(coefficientOfFriction);
	result["collider"] = colliderToJson(collider);
	return result;
}

auto BodyEditor::fromJson(const Json::Value& json) -> BodyEditor {
	return BodyEditor{
		.pos = Vec2{ json.at("pos").at("x").number(), json.at("pos").at("y").number() },
		.orientation = json.at("orientation").number(),
		.vel = Vec2{ json.at("vel").at("x").number(), json.at("vel").at("y").number() },
		.angularVel = json.at("angularVel").number(),
		.mass = json.at("mass").number(),
		.rotationalInertia = json.at("rotationalInertia").number(),
		.coefficientOfFriction = json.at("coefficientOfFriction").number(),
		.collider = jsonToCollider(json.at("collider")),
	};
}

