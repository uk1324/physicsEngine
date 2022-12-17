#include <game\collidersData.hpp>
#include <game/editor/commands.hpp>
#include <imgui/imgui.h>
#include <utils/io.hpp>

using namespace ImGui;
using namespace Json;

auto CircleColliderEditor::editorGui(EditorGuiState& inputState, EditorEntities& entites, const Entity& entity, Commands& commands) -> void {
	InputFloat("radius", &radius);
	if (IsItemActivated()) {
 		inputState.inputing = true;
		*reinterpret_cast<decltype(radius)*>(inputState.placeToSaveDataAfterNewChange()) = radius;
	}
	if (IsItemDeactivatedAfterEdit()) {
		dbg(*reinterpret_cast<decltype(radius)*>(inputState.oldSavedData()));
		commands.addSetFieldCommand(entity, CIRCLE_COLLIDER_EDITOR_RADIUS_OFFSET, inputState.oldSavedData(), entites.getFieldPointer(entity, CIRCLE_COLLIDER_EDITOR_RADIUS_OFFSET), static_cast<u8>(sizeof(radius)));
	}
	if (IsItemDeactivated()) { inputState.inputing = false; }

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

auto BoxColliderEditor::editorGui(EditorGuiState& inputState, EditorEntities& entites, const Entity& entity, Commands& commands) -> void {
	InputFloat2("size", size.data());
	if (IsItemActivated()) {
 		inputState.inputing = true;
		*reinterpret_cast<decltype(size)*>(inputState.placeToSaveDataAfterNewChange()) = size;
	}
	if (IsItemDeactivatedAfterEdit()) {
		dbg(*reinterpret_cast<decltype(size)*>(inputState.oldSavedData()));
		commands.addSetFieldCommand(entity, BOX_COLLIDER_EDITOR_SIZE_OFFSET, inputState.oldSavedData(), entites.getFieldPointer(entity, BOX_COLLIDER_EDITOR_SIZE_OFFSET), static_cast<u8>(sizeof(size)));
	}
	if (IsItemDeactivated()) { inputState.inputing = false; }

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

