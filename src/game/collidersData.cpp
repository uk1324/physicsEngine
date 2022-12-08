#include <game\collidersData.hpp>
#include <imgui/imgui.h>

using namespace ImGui;
using namespace Json;

auto CircleColliderEditor::displayGui() -> void {
	InputFloat("radius", &radius);
}

auto CircleColliderEditor::toJson() const -> Json::Value {
	auto result = Json::Value::emptyObject();
	result["radius"] = Json::Value(radius);
	return result;
}

auto CircleColliderEditor::fromJson(const Json::Value& json) -> CircleColliderEditor {
	return CircleColliderEditor{
		.radius = json.at("radius").number(),
	};
}

auto BoxColliderEditor::displayGui() -> void {
	InputFloat2("size", size.data());
}

auto BoxColliderEditor::toJson() const -> Json::Value {
	auto result = Json::Value::emptyObject();
	result["size"] = { { "x", size.x }, { "y", size.y } };
	return result;
}

auto BoxColliderEditor::fromJson(const Json::Value& json) -> BoxColliderEditor {
	return BoxColliderEditor{
		.size = Vec2{ json.at("size").at("x").number(), json.at("size").at("y").number() },
	};
}

