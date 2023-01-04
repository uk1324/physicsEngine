#include <game/editor/selectedEntityGizmo.hpp>
#include <game/editor/commands.hpp>
#include <game/editor/customGuis.hpp>
#include <game/debug.hpp>
#include <game/editor/input.hpp>
#include <math/lineSegment.hpp>
#include <math/mat2.hpp>
#include <game/collision/collision.hpp>

#include <imgui/imgui.h>

static const auto ROTATION_GIZMO_COLOR = Vec3::WHITE / 2.0f;

#include <utils/io.hpp>

auto SelectedEntityGizmo::update(
	EditorEntities& entites,
	Commands& commands,
	const Camera& camera,
	const std::vector<Entity>& selectedEntities,
	Vec2 selectedEntitiesCenterPos,
	Vec2 cursorPos) -> bool {

	// This has to be set even if selectedEntities.empty() to allow update and draw to be called in any order. Otherwise it wouldn't be initalized to the most up to date values, which causes gliches when selecting for the first time or after zooming.
	undoZoomScale = 1.0f / (camera.zoom * 8.0f);
	xyAxisGizmosLength = undoZoomScale;
	bothGizmoBoxSize = xyAxisGizmosLength / 4.0f;
	rotationGizmoRadius = xyAxisGizmosLength / 2.0f;

	if (selectedEntities.empty()) {
		selectedGizmo = GizmoType::NONE;
		return false;
	}

	LineSegment
		xAxisLineSegment{ selectedEntitiesCenterPos, selectedEntitiesCenterPos + xAxis * xyAxisGizmosLength },
		yAxisLineSegment{ selectedEntitiesCenterPos, selectedEntitiesCenterPos + yAxis * xyAxisGizmosLength };

	auto selectedEntitesChanged = selectedEntities != grabStartSelectedEntities;
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
			selectedEntitesChanged = true;
		}
	}

	if (selectedGizmo != GizmoType::NONE && 
		(Input::isMouseButtonUp(MouseButton::LEFT) 
		|| (selectedEntitesGrabStartPositions.size() == grabStartSelectedEntities.size() && selectedEntities != grabStartSelectedEntities))) {
		commands.beginMulticommand();
		for (usize i = 0; i < selectedEntitesGrabStartPositions.size(); i++) {
			const auto& entity = grabStartSelectedEntities[i];
			const auto posOffset = entites.getPosPointerOffset(entity);
			if (posOffset.has_value()) {
				const auto newPos = entites.getPosOrOrigin(entity);
				const auto oldPos = selectedEntitesGrabStartPositions[i];
				if (newPos != oldPos) {
					commands.addSetFieldCommand(entity, *posOffset, &oldPos, &newPos, sizeof(newPos));
				}
			}
		}

		// These have to be 2 separate loops because not all transformations require orientations.
		if (grabStartSelectedEntities.size() == selectedEntitesGrabStartOrientations.size()) {
			for (usize i = 0; i < selectedEntitesGrabStartOrientations.size(); i++) {
				const auto& entity = grabStartSelectedEntities[i];
				const auto orientationOffset = entites.getOrientationPointerOffset(entity);
				if (orientationOffset.has_value()) {
					const auto newOrientation = entites.getOrientationOrZero(entity);
					const auto oldOrientation = selectedEntitesGrabStartOrientations[i];
					if (newOrientation != oldOrientation) {
						commands.addSetFieldCommand(entity, *orientationOffset, &oldOrientation, &newOrientation, sizeof(newOrientation));
					}
				}
			}
		}
		commands.endMulticommand();
	}

	if (selectedEntitesChanged) {
		grabStartSelectedEntities = selectedEntities;
		axisGrabStartPos = cursorPos;

		if (selectedGizmo != GizmoType::NONE) {
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


			// In unity moving left and right rotates the object. This might be a better option for 3D but I thing using the actual angle works better in 2D.
			const auto startAngle = atan2(centerToOldPos.y, centerToOldPos.x);
			auto angleDifference = atan2(centerToNewPos.y, centerToNewPos.x) - startAngle;

			if (Input::isButtonHeld(EditorButton::GIZMO_GRID_SNAP)) {
				angleDifference -= fmod(angleDifference, rotationGridCellSize);
			}

			// Putting this into the draw method would require creating new variables with confusing names. The only reason for a separate draw method is to avoid lag which doesn't matter here.
			Debug::drawRay(center, centerToOldPos * rotationGizmoRadius, ROTATION_GIZMO_COLOR);
			Debug::drawRay(center, Vec2::oriented(startAngle + angleDifference) * rotationGizmoRadius, ROTATION_GIZMO_COLOR);

			ASSERT(grabStartSelectedEntities.size() == selectedEntitesGrabStartPositions.size());
			for (usize i = 0; i < selectedEntities.size(); i++) {
				Vec2 p = selectedEntitesGrabStartPositions[i];
				p -= center;
				p *= Mat2::rotate(angleDifference);
				p += center;
				entites.setPos(grabStartSelectedEntities[i], p);

				entites.setOrientation(grabStartSelectedEntities[i], selectedEntitesGrabStartOrientations[i] + angleDifference);
			}

		} else if (selectedGizmo != GizmoType::NONE) {
			Vec2 translation{ 0.0f };
			if (selectedGizmo == GizmoType::X)
				translation = xAxis * xAxisLineSegment.line.distanceAlong(grabDifference);
			else if (selectedGizmo == GizmoType::Y)
				translation = yAxis * yAxisLineSegment.line.distanceAlong(grabDifference);
			else if (selectedGizmo == GizmoType::BOTH)
				translation = grabDifference;

			if (Input::isButtonHeld(EditorButton::GIZMO_GRID_SNAP)) {
				const Vec2 dist{ dot(xAxis, translation), det(xAxis, translation) };
				const Vec2 distSnapped = dist - Vec2{ fmod(dist.x, gridCellSize),  fmod(dist.y, gridCellSize) };
				translation = xAxis * distSnapped.x + yAxis * distSnapped.y;
			}

			ASSERT(grabStartSelectedEntities.size() == selectedEntitesGrabStartPositions.size());
			for (usize i = 0; i < grabStartSelectedEntities.size(); i++) {
				entites.setPos(grabStartSelectedEntities[i], selectedEntitesGrabStartPositions[i] + translation);
			}
		}
	}

	if (Input::isMouseButtonUp(MouseButton::LEFT)) {
		const auto wasSelected = selectedGizmo != GizmoType::NONE;
		selectedGizmo = GizmoType::NONE;

		const auto selectedEntitesChanged = selectedEntities.size() != selectedEntitesGrabStartPositions.size();
		// Don't need to save a command if the LEFT up is caused by selecting different entites. Alternatively could store a flag if the gizmo is being draged and check it.
		if (selectedEntitesChanged)
			return wasSelected;

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

auto SelectedEntityGizmo::settingsGui() -> void {
	using namespace ImGui;
	auto angle = atan2(xAxis.y, xAxis.x);
	if (inputAngle("axis gizmos angle", &angle)) {
		xAxis = Vec2::oriented(angle);
		yAxis = Vec2::oriented(angle + PI<float> / 2.0f);
	}
}
