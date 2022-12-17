#include <game/editor/selectedEntityGizmo.hpp>
#include <game/debug.hpp>
#include <game/editor/input.hpp>
#include <math/lineSegment.hpp>
#include <math/mat2.hpp>
#include <game/collision/collision.hpp>

static const auto ROTATION_GIZMO_COLOR = Vec3::WHITE / 2.0f;


auto SelectedEntityGizmo::update(
	EditorEntities& entites,
	const Camera& camera,
	const std::vector<Entity>& selectedEntities,
	Vec2 selectedEntitiesCenterPos,
	Vec2 cursorPos) -> bool {

	if (selectedEntities.empty()) {
		selectedGizmo = GizmoType::NONE;
		return false;
	}

	undoZoomScale = 1.0f / (camera.zoom * 8.0f);
	xyAxisGizmosLength = undoZoomScale;
	bothGizmoBoxSize = xyAxisGizmosLength / 4.0f;
	rotationGizmoRadius = xyAxisGizmosLength / 2.0f;

	LineSegment
		xAxisLineSegment{ selectedEntitiesCenterPos, selectedEntitiesCenterPos + xAxis * xyAxisGizmosLength },
		yAxisLineSegment{ selectedEntitiesCenterPos, selectedEntitiesCenterPos + yAxis * xyAxisGizmosLength };

	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		// TODO: This and radius is dependent on zoom.
		const auto axisGizmoWidth = 0.15f * undoZoomScale;
		const auto rotationGizmoHalfWidth = 0.15f * undoZoomScale;
		const auto boxCenter = selectedEntitiesCenterPos + xAxis * bothGizmoBoxSize / 2.0f + yAxis * bothGizmoBoxSize  / 2.0f;

		selectedGizmo = GizmoType::NONE;
		if (contains(cursorPos, boxCenter, 0.0f, BoxCollider{ BoxColliderEditor{ .size = Vec2{ bothGizmoBoxSize } } })) {
			selectedGizmo = GizmoType::BOTH;
		} else if (xAxisLineSegment.asBoxContains(axisGizmoWidth, cursorPos)) {
			selectedGizmo = GizmoType::X;
		} else if (yAxisLineSegment.asBoxContains(axisGizmoWidth, cursorPos)) {
			selectedGizmo = GizmoType::Y;
		} else if (const auto dist = distance(cursorPos, selectedEntitiesCenterPos);
			dist > rotationGizmoRadius - rotationGizmoHalfWidth && dist < rotationGizmoRadius + rotationGizmoHalfWidth) {
			selectedGizmo = GizmoType::ROTATION;
		}

		if (selectedGizmo != GizmoType::NONE) {
			axisGrabStartPos = cursorPos;
			selectedEntitesGrabStartPositions.clear();
			for (const auto& entity : selectedEntities) {
				selectedEntitesGrabStartPositions.push_back(entites.getPosOrOrigin(entity));
			}
		}

		if (selectedGizmo == GizmoType::ROTATION) {
			// Don't really have to do this if there is only one entity.
			selectedEntitesGrabStartOrientations.clear();
			for (const auto& entity : selectedEntities) {
				selectedEntitesGrabStartOrientations.push_back(entites.getOrientationOrZero(entity));
			}
		}
	}

	if (Input::isMouseButtonHeld(MouseButton::LEFT)) {
		const auto grabDifference = cursorPos - axisGrabStartPos;
		if (selectedGizmo == GizmoType::ROTATION) {
			// Because of some bug rotating around the current center of the selected items sometimes translates it so this just uses the center when the entites were first selected. 
			Vec2 center{ 0.0f };
			for (const auto& pos : selectedEntitesGrabStartPositions) {
				center += pos;
			}
			center /= static_cast<float>(selectedEntitesGrabStartPositions.size());

			// Assumes the center doesn't get translated during a rotation.
			const auto centerToOldPos = (axisGrabStartPos - center).normalized();
			const auto centerToNewPos = (cursorPos - center).normalized();

			// Putting this into the draw method would require creating new variables with confusing names. The only reason for a separate draw method is to avoid lag which doesn't matter here.
			Debug::drawRay(center, centerToOldPos * rotationGizmoRadius, ROTATION_GIZMO_COLOR);
			Debug::drawRay(center, centerToNewPos * rotationGizmoRadius, ROTATION_GIZMO_COLOR);

			// In unity moving left and right rotates the object. This might be a better option for 3D but I thing using the actual angle works better in 2D.
			auto angleDifference = atan2(centerToNewPos.y, centerToNewPos.x) - atan2(centerToOldPos.y, centerToOldPos.x);

			ASSERT(selectedEntities.size() == selectedEntitesGrabStartPositions.size());
			for (usize i = 0; i < selectedEntities.size(); i++) {
				Vec2 p = selectedEntitesGrabStartPositions[i];
				p -= center;
				p *= Mat2::rotate(angleDifference);
				p += center;
				entites.setPos(selectedEntities[i], p);

				entites.setOrientation(selectedEntities[i], selectedEntitesGrabStartOrientations[i] + angleDifference);
			}

		} else if (selectedGizmo != GizmoType::NONE) {
			Vec2 translation{ 0.0f };
			if (selectedGizmo == GizmoType::X)
				translation = xAxis * xAxisLineSegment.line.distanceAlong(grabDifference);
			else if (selectedGizmo == GizmoType::Y)
				translation = yAxis * yAxisLineSegment.line.distanceAlong(grabDifference);
			else if (selectedGizmo == GizmoType::BOTH)
				translation = grabDifference;

			ASSERT(selectedEntities.size() == selectedEntitesGrabStartPositions.size());
			for (usize i = 0; i < selectedEntities.size(); i++) {
				entites.setPos(selectedEntities[i], selectedEntitesGrabStartPositions[i] + translation);
			}
		}
	}

	if (Input::isMouseButtonUp(MouseButton::LEFT)) {
		const auto wasSelected = selectedGizmo != GizmoType::NONE;
		selectedGizmo = GizmoType::NONE;
		return wasSelected;
	}

	return selectedGizmo != GizmoType::NONE;
}

auto SelectedEntityGizmo::draw(Vec2 selectedEntitiesCenterPos) -> void {
	const auto v0 = xAxis * bothGizmoBoxSize, v1 = yAxis * bothGizmoBoxSize;
	const auto BLUE = Vec3{ 171.0f, 218.0f, 255.0f } / 255.0f;
	Debug::drawRay(selectedEntitiesCenterPos + v0, v1, BLUE);
	Debug::drawRay(selectedEntitiesCenterPos + v1, v0, BLUE);

	Debug::drawRay(selectedEntitiesCenterPos, xAxis * xyAxisGizmosLength, Vec3::RED);
	Debug::drawRay(selectedEntitiesCenterPos, yAxis * xyAxisGizmosLength, Vec3::GREEN);

	Debug::drawHollowCircle(selectedEntitiesCenterPos, rotationGizmoRadius, ROTATION_GIZMO_COLOR);
}