#pragma once

#include <game/renderer.hpp>
#include <game/editor/selectedEntityGizmo.hpp>
#include <game/editor/commands.hpp>
#include <game/editor/editorGuiState.hpp>

class Editor {
public:
	Editor();
	auto update(Gfx& gfx, Renderer& renderer) -> void;

private:
	auto getCursorPos() -> Vec2;
	
	// Maybe make this a single struct.
	std::vector<Entity> selectedEntities;
	auto addToSelectedEntities(const Entity& entity) -> void;
	auto updateSelectedEntitesCenterPos() -> void;
	Vec2 selectedEntitiesCenterPos;
	std::vector<Entity> lastSelectedEntitesUnderCursor;
	std::vector<Entity> currentSelectedEntitesUnderCursor;
	usize currentEntityIndexInSelectCycle = 0;

	Vec2 screenGrabStartPos;
	Vec2 centerGrabStartPos;

	Vec2 selectGrabStartPos;
	bool selecting = false;

	EditorEntities entites;
	SelectedEntityGizmo selectedEntityGizmo;

	auto undoCommand(const Command& command) -> void;
	auto redoCommand(const Command& command) -> void;
	EditorGuiState guiState;
	Commands commands;

	Camera camera;
};