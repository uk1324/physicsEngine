#include <game\entitesData.hpp>
#include <game/editor/commands.hpp>
#include <imgui/imgui.h>
#include <utils/io.hpp>

using namespace ImGui;
using namespace Json;

auto BodyEditor::editorGui(EditorGuiState& inputState, EditorEntities& entites, const Entity& entity, Commands& commands) -> void {
	InputFloat2("pos", pos.data());
	if (IsItemActivated()) {
 		inputState.inputing = true;
		*reinterpret_cast<decltype(pos)*>(inputState.placeToSaveDataAfterNewChange()) = pos;
	}
	if (IsItemDeactivatedAfterEdit()) {
		dbg(*reinterpret_cast<decltype(pos)*>(inputState.oldSavedData()));
		commands.addSetFieldCommand(entity, BODY_EDITOR_POS_OFFSET, inputState.oldSavedData(), entites.getFieldPointer(entity, BODY_EDITOR_POS_OFFSET), static_cast<u8>(sizeof(pos)));
	}
	if (IsItemDeactivated()) { inputState.inputing = false; }

	inputAngle("orientation", &orientation);
	if (IsItemActivated()) {
 		inputState.inputing = true;
		*reinterpret_cast<decltype(orientation)*>(inputState.placeToSaveDataAfterNewChange()) = orientation;
	}
	if (IsItemDeactivatedAfterEdit()) {
		dbg(*reinterpret_cast<decltype(orientation)*>(inputState.oldSavedData()));
		commands.addSetFieldCommand(entity, BODY_EDITOR_ORIENTATION_OFFSET, inputState.oldSavedData(), entites.getFieldPointer(entity, BODY_EDITOR_ORIENTATION_OFFSET), static_cast<u8>(sizeof(orientation)));
	}
	if (IsItemDeactivated()) { inputState.inputing = false; }

	InputFloat2("vel", vel.data());
	if (IsItemActivated()) {
 		inputState.inputing = true;
		*reinterpret_cast<decltype(vel)*>(inputState.placeToSaveDataAfterNewChange()) = vel;
	}
	if (IsItemDeactivatedAfterEdit()) {
		dbg(*reinterpret_cast<decltype(vel)*>(inputState.oldSavedData()));
		commands.addSetFieldCommand(entity, BODY_EDITOR_VEL_OFFSET, inputState.oldSavedData(), entites.getFieldPointer(entity, BODY_EDITOR_VEL_OFFSET), static_cast<u8>(sizeof(vel)));
	}
	if (IsItemDeactivated()) { inputState.inputing = false; }

	InputFloat("angularVel", &angularVel);
	if (IsItemActivated()) {
 		inputState.inputing = true;
		*reinterpret_cast<decltype(angularVel)*>(inputState.placeToSaveDataAfterNewChange()) = angularVel;
	}
	if (IsItemDeactivatedAfterEdit()) {
		dbg(*reinterpret_cast<decltype(angularVel)*>(inputState.oldSavedData()));
		commands.addSetFieldCommand(entity, BODY_EDITOR_ANGULAR_VEL_OFFSET, inputState.oldSavedData(), entites.getFieldPointer(entity, BODY_EDITOR_ANGULAR_VEL_OFFSET), static_cast<u8>(sizeof(angularVel)));
	}
	if (IsItemDeactivated()) { inputState.inputing = false; }

	InputFloat("mass", &mass);
	if (IsItemActivated()) {
 		inputState.inputing = true;
		*reinterpret_cast<decltype(mass)*>(inputState.placeToSaveDataAfterNewChange()) = mass;
	}
	if (IsItemDeactivatedAfterEdit()) {
		dbg(*reinterpret_cast<decltype(mass)*>(inputState.oldSavedData()));
		commands.addSetFieldCommand(entity, BODY_EDITOR_MASS_OFFSET, inputState.oldSavedData(), entites.getFieldPointer(entity, BODY_EDITOR_MASS_OFFSET), static_cast<u8>(sizeof(mass)));
	}
	if (IsItemDeactivated()) { inputState.inputing = false; }

	InputFloat("rotationalInertia", &rotationalInertia);
	if (IsItemActivated()) {
 		inputState.inputing = true;
		*reinterpret_cast<decltype(rotationalInertia)*>(inputState.placeToSaveDataAfterNewChange()) = rotationalInertia;
	}
	if (IsItemDeactivatedAfterEdit()) {
		dbg(*reinterpret_cast<decltype(rotationalInertia)*>(inputState.oldSavedData()));
		commands.addSetFieldCommand(entity, BODY_EDITOR_ROTATIONAL_INERTIA_OFFSET, inputState.oldSavedData(), entites.getFieldPointer(entity, BODY_EDITOR_ROTATIONAL_INERTIA_OFFSET), static_cast<u8>(sizeof(rotationalInertia)));
	}
	if (IsItemDeactivated()) { inputState.inputing = false; }

	InputFloat("coefficientOfFriction", &coefficientOfFriction);
	if (IsItemActivated()) {
 		inputState.inputing = true;
		*reinterpret_cast<decltype(coefficientOfFriction)*>(inputState.placeToSaveDataAfterNewChange()) = coefficientOfFriction;
	}
	if (IsItemDeactivatedAfterEdit()) {
		dbg(*reinterpret_cast<decltype(coefficientOfFriction)*>(inputState.oldSavedData()));
		commands.addSetFieldCommand(entity, BODY_EDITOR_COEFFICIENT_OF_FRICTION_OFFSET, inputState.oldSavedData(), entites.getFieldPointer(entity, BODY_EDITOR_COEFFICIENT_OF_FRICTION_OFFSET), static_cast<u8>(sizeof(coefficientOfFriction)));
	}
	if (IsItemDeactivated()) { inputState.inputing = false; }

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

auto DistanceJointAnchorEditor::editorGui(EditorGuiState& inputState, EditorEntities& entites, const Entity& entity, Commands& commands) -> void {
	;
	if (IsItemActivated()) {
 		inputState.inputing = true;
		*reinterpret_cast<decltype(body)*>(inputState.placeToSaveDataAfterNewChange()) = body;
	}
	if (IsItemDeactivatedAfterEdit()) {
		dbg(*reinterpret_cast<decltype(body)*>(inputState.oldSavedData()));
		commands.addSetFieldCommand(entity, DISTANCE_JOINT_ANCHOR_EDITOR_BODY_OFFSET, inputState.oldSavedData(), entites.getFieldPointer(entity, DISTANCE_JOINT_ANCHOR_EDITOR_BODY_OFFSET), static_cast<u8>(sizeof(body)));
	}
	if (IsItemDeactivated()) { inputState.inputing = false; }

	InputFloat2("objectSpaceOffset", objectSpaceOffset.data());
	if (IsItemActivated()) {
 		inputState.inputing = true;
		*reinterpret_cast<decltype(objectSpaceOffset)*>(inputState.placeToSaveDataAfterNewChange()) = objectSpaceOffset;
	}
	if (IsItemDeactivatedAfterEdit()) {
		dbg(*reinterpret_cast<decltype(objectSpaceOffset)*>(inputState.oldSavedData()));
		commands.addSetFieldCommand(entity, DISTANCE_JOINT_ANCHOR_EDITOR_OBJECT_SPACE_OFFSET_OFFSET, inputState.oldSavedData(), entites.getFieldPointer(entity, DISTANCE_JOINT_ANCHOR_EDITOR_OBJECT_SPACE_OFFSET_OFFSET), static_cast<u8>(sizeof(objectSpaceOffset)));
	}
	if (IsItemDeactivated()) { inputState.inputing = false; }

}

auto DistanceJointAnchorEditor::toJson() const -> Json::Value {
	auto result = Json::Value::emptyObject();
	result["body"] = Json::Value(static_cast<Json::Value::IntType>(body));
	result["objectSpaceOffset"] = { { "x", objectSpaceOffset.x }, { "y", objectSpaceOffset.y } };
	return result;
}

auto DistanceJointAnchorEditor::fromJson(const Json::Value& json) -> DistanceJointAnchorEditor {
	return DistanceJointAnchorEditor{
		.body = static_cast<usize>(json.at("body").intNumber()),
		.objectSpaceOffset = Vec2{ json.at("objectSpaceOffset").at("x").number(), json.at("objectSpaceOffset").at("y").number() },
	};
}

auto DistanceJointEntityEditor::editorGui(EditorGuiState& inputState, EditorEntities& entites, const Entity& entity, Commands& commands) -> void {
	anchorA.editorGui(inputState, entites, entity, commands);
	displayAnchorGui(staticWorldSpaceAnchorOrBodyAnchorB);
	InputFloat("distance", &distance);
	if (IsItemActivated()) {
 		inputState.inputing = true;
		*reinterpret_cast<decltype(distance)*>(inputState.placeToSaveDataAfterNewChange()) = distance;
	}
	if (IsItemDeactivatedAfterEdit()) {
		dbg(*reinterpret_cast<decltype(distance)*>(inputState.oldSavedData()));
		commands.addSetFieldCommand(entity, DISTANCE_JOINT_ENTITY_EDITOR_DISTANCE_OFFSET, inputState.oldSavedData(), entites.getFieldPointer(entity, DISTANCE_JOINT_ENTITY_EDITOR_DISTANCE_OFFSET), static_cast<u8>(sizeof(distance)));
	}
	if (IsItemDeactivated()) { inputState.inputing = false; }

}

auto DistanceJointEntityEditor::toJson() const -> Json::Value {
	auto result = Json::Value::emptyObject();
	result["anchorA"] = anchorA.toJson();
	result["staticWorldSpaceAnchorOrBodyAnchorB"] = anchorToJson(staticWorldSpaceAnchorOrBodyAnchorB);
	result["distance"] = Json::Value(distance);
	return result;
}

auto DistanceJointEntityEditor::fromJson(const Json::Value& json) -> DistanceJointEntityEditor {
	return DistanceJointEntityEditor{
		.staticWorldSpaceAnchorOrBodyAnchorB = jsonToAnchor(json.at("staticWorldSpaceAnchorOrBodyAnchorB")),
		.distance = json.at("distance").number(),
	};
}

