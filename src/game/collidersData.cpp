#include <game\collidersData.hpp>
#include <imgui/imgui.h>

using namespace ImGui;
using namespace Json;

auto CircleCollider::displayGui() -> void {
	InputFloat("radius", &radius);
}
auto CircleCollider::toJson() const -> Json::Value {
	auto result = Json::Value::emptyObject();
	result["radius"] = Json::Value(radius);
	return result;
}
auto CircleCollider::fromJson(const Json::Value& json) -> CircleCollider {
	CircleCollider result;
	result.radius = json.at("radius").number();
	return result;
}
auto BoxCollider::displayGui() -> void {
	InputFloat2("size", size.data());
}
auto BoxCollider::toJson() const -> Json::Value {
	auto result = Json::Value::emptyObject();
	result["size"] = { { "x", size.x }, { "y", size.y } };
	return result;
}
auto BoxCollider::fromJson(const Json::Value& json) -> BoxCollider {
	BoxCollider result;
	result.size = Vec2{ json.at("size").at("x").number(), json.at("size").at("y").number() };
	return result;
}
