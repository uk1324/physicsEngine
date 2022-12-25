#pragma once

#include <game/entitesData.hpp>
#include <game/editor/editorEntity.hpp>
#include <game/renderer.hpp>
#include <math/utils.hpp>

/*
Using a class for this instead of just storing it in the editor is bettern, because 
you can limit the amount of state it can access
there can be more than one instance although this isn't really useful here
update and draw share state. Could just make functions that recalculate it but this might make it harder to read. Update and draw have to be separate so draw doesn't lag begind.
The issue is that all the state has to be passed into the methods.

*/
class SelectedEntityGizmo {
public:
	auto update(
		EditorEntities& entites,
		Commands& commands,
		const Camera& camera, 
		const std::vector<Entity>& selectedEntities,
		Vec2 selectedEntitiesCenterPos, 
		Vec2 cursorPos) -> bool;
	auto draw(Vec2 selectedEntitiesCenterPos) -> void;
	auto settingsGui() -> void;

	// Has to be normalized.
	Vec2 xAxis{ 1.0f, 0.0f };
	Vec2 yAxis{ 0.0f, 1.0f };

	float gridCellSize = 0.25f;
	float rotationGridCellSize = PI<float> / 8.0f;

private:
	float undoZoomScale;
	float xyAxisGizmosLength;
	float bothGizmoBoxSize;
	float rotationGizmoRadius;

	enum class GizmoType {
		X, Y, BOTH, NONE, ROTATION
	};
	GizmoType selectedGizmo = GizmoType::NONE;
	Vec2 axisGrabStartPos;

	// Using a union or std::variant might be better organization-wise, but it would also require deallcating and allocating objects on change.

	// X, Y, BOTH, ROTATION
	std::vector<Vec2> selectedEntitesGrabStartPositions;

	// ROTATION
	std::vector<float> selectedEntitesGrabStartOrientations;
};