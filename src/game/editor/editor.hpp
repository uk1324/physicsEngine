#pragma once

#include <game/renderer.hpp>
#include <game/editor/selectedEntityGizmo.hpp>

// TODO: Evaluating expressions in ImGui inputs. Could also include access to variables.
class Editor {
public:
	Editor();
	auto update(Gfx& gfx, Renderer& renderer) -> void;

private:
	// Cycling selecting can't just be implemented by comparing with the slected item because this would only work if there were 2 overlapping entites.

	auto getCursorPos() -> Vec2;
	
	// Maybe make this a single struct.
	std::vector<Entity> selectedEntities;
	auto updateSelectedEntitesCenterPos() -> void;
	Vec2 selectedEntitiesCenterPos;

	Vec2 screenGrabStartPos;
	Vec2 centerGrabStartPos;

	EditorEntites entites;
	SelectedEntityGizmo selectedEntityGizmo;

	Camera camera;
};