#pragma once

#include <game/renderer.hpp>
#include <game/editor/selectedEntityGizmo.hpp>
#include <game/editor/commands.hpp>
#include <game/editor/editorGuiState.hpp>

class Editor {
public:
	Editor();
	auto registerInputButtons() -> void;
	auto update(Gfx& gfx, Renderer& renderer) -> void;

private:
	auto getCursorPos() -> Vec2;
	
	// Maybe make this a single struct.
	std::vector<Entity> selectedEntities;
	auto addToSelectedEntities(const Entity& entity) -> void;
	auto updateSelectedEntitesData() -> void;
	Vec2 selectedEntitiesCenterPos;
	Vec2 lastFrameSelectedEntitiesCenterPos;
	bool selectedEntitesMoved = false;
	Aabb selectedEntitesAabb{ Vec2{ 0.0f }, Vec2{ 0.0f } };
	bool selectedEntitesChanged = false;
	std::vector<Entity> lastSelectedEntitesUnderCursor;
	std::vector<Entity> currentSelectedEntitesUnderCursor;
	usize currentEntityIndexInSelectCycle = 0;

	Vec2 lastFrameFocusPos{ 0.0f };
	float elapsedSinceFocusStart = 0.0f;
	bool focusing = false;

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