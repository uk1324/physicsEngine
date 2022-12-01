#include <game\entitesData.hpp>
#include <imgui/imgui.h>

using namespace ImGui;
using namespace Json;

auto Body::displayGui() -> void {
	InputFloat2("pos", pos.data());
	InputFloat("orientation", &orientation);
	InputFloat("coefficientOfFriction", &coefficientOfFriction);
	InputFloat("mass", &mass);
	InputFloat("rotationalInertia", &rotationalInertia);
	displayColliderGui(collider);
}
auto Body::toJson() const -> Json::Value {
	auto result = Json::Value::emptyObject();
	result["pos"] = { { "x", pos.x }, { "y", pos.y } };
	result["orientation"] = Json::Value(orientation);
	result["coefficientOfFriction"] = Json::Value(coefficientOfFriction);
	result["mass"] = Json::Value(mass);
	result["rotationalInertia"] = Json::Value(rotationalInertia);
	result["collider"] = colliderToJson(collider);
	return result;
}
auto Body::fromJson(const Json::Value& json) -> Body {
	Body result;
	result.pos = Vec2{ json.at("pos").at("x").number(), json.at("pos").at("y").number() };
	result.orientation = json.at("orientation").number();
	result.coefficientOfFriction = json.at("coefficientOfFriction").number();
	result.mass = json.at("mass").number();
	result.rotationalInertia = json.at("rotationalInertia").number();
	result.collider = jsonToCollider(json.at("collider"));
	return result;
}
