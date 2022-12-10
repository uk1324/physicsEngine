#include <game/editor/editor.hpp>
#include <game/debug.hpp>
#include <imgui/imgui.h>
#include <game/editor/input.hpp>
#include <game/collision/collision.hpp>
#include <engine/time.hpp>
#include <math/lineSegment.hpp>
#include <math/mat2.hpp>
#include <engine/window.hpp>
#include <utils/io.hpp>

Editor::Editor() {
	camera.zoom = 0.125f / 2.0f;
	camera.pos = Vec2{ 0.0f, 0.0f };

	//Input::registerKeyButton()
}

auto Editor::update(Gfx& gfx, Renderer& renderer) -> void {
	camera.aspectRatio = Window::aspectRatio();

	using namespace ImGui;
	Begin("entites");

	if (Button("+")) {
		entites.entitesBody.push_back(BodyEditor{
			.pos = Vec2{ camera.pos },
			.mass = 20.0f,
			.rotationalInertia = 1.0f,
			.collider = CircleColliderEditor{ 1.0f }
		});
	}
	End();

	Begin("selected");
	for (const auto& entity : selectedEntities) {
		switch (entity.type) {
		case EntityType::Null: ImGui::Text("no entity selected"); break;
		case EntityType::Body: entites.entitesBody[entity.index].displayGui(); break;
		}
	}
	End();

	const auto lengthScale = 1.0f / (camera.zoom * 8.0f);
	// Rename to xAxisGizmo
	Vec2 xAxis{ 1.0f * lengthScale, 0.0f };
	Vec2 yAxis{ 0.0f, 1.0f * lengthScale };

	/*
	* checkGrabTranslationGizmo
	if (!grabTranslationGizmo()) {
		grabEntity
		...
	}
	*/
	// Rotation gizmo: just draw a cricle. Should it be a SelectedAxis or something else? There shouldn't be a way to select 2 at one time.
	// Chain editor. Half spaces or lines?
	// Commands: pool of (commands of commands)
	// Proceduraly generating terrain under the chain.
	// History
	// Shortcuts.
	// Cycling select.
	//static constexpr float AXIS_BOTH_LENGTH_SCALE = 1.0f / 4.0f;
	//const auto rotationGizmoRadius = lengthScale / 2.0f;
	//// Could put this into a class instead of a function.
	//auto gizmos = [&, this]() -> bool {
	//	Vec2 
	//		xAxisNormalized = xAxis.normalized(),
	//		yAxisNormalized = yAxis.normalized();
	//	LineSegment 
	//		xAxisLineSegment{ selectedEntitiesCenterPos, selectedEntitiesCenterPos + xAxis },
	//		yAxisLineSegment{ selectedEntitiesCenterPos, selectedEntitiesCenterPos + yAxis };

	//	auto gizmoSelected = false;
	//	const auto cursorPos = getCursorPos();
	//	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
	//		if (!selectedEntities.empty()) {
	//			// TODO: This and radius is dependent on zoom.
	//			static constexpr auto LINE_WIDTH = 0.2f;
	//			selectedGizmo = GizmoType::NONE;
	//			const auto boxCenter = selectedEntitiesCenterPos + xAxis * AXIS_BOTH_LENGTH_SCALE / 2.0f + yAxis * AXIS_BOTH_LENGTH_SCALE / 2.0f;
	//			const auto boxSize = Vec2{ xAxis.length(), yAxis.length() } * AXIS_BOTH_LENGTH_SCALE;
	//			if (contains(cursorPos, boxCenter, 0.0f, BoxCollider{ BoxColliderEditor{ .size = boxSize } })) {
	//				selectedGizmo = GizmoType::BOTH;
	//			} else if (xAxisLineSegment.asBoxContains(LINE_WIDTH, cursorPos)) {
	//				selectedGizmo = GizmoType::X;
	//			} else if (yAxisLineSegment.asBoxContains(LINE_WIDTH, cursorPos)) {
	//				selectedGizmo = GizmoType::Y;
	//			} else if (const auto dist = distance(cursorPos, selectedEntitiesCenterPos); 
	//				dist > rotationGizmoRadius - 0.05f && dist < rotationGizmoRadius + 0.05f) {
	//				selectedGizmo = GizmoType::ROTATION;
	//			}

	//			if (selectedGizmo != GizmoType::NONE) {
	//				axisGrabStartPos = cursorPos;
	//				selectedEntitesGrabStartPositions.clear();
	//				for (const auto& entity : selectedEntities) {
	//					selectedEntitesGrabStartPositions.push_back(getEntityPosOrOrigin(entity));
	//				}
	//				gizmoSelected = true;
	//			}
	//			
	//			if (selectedGizmo == GizmoType::ROTATION) {
	//				// Don't really have to do this if there is only one entity.
	//				for (const auto& entity : selectedEntities) {
	//					selectedEntitesGrabStartOrientations.push_back(getEntityPosOrZero(entity));
	//				}
	//			}
	//		}
	//	}

	//	if (Input::isMouseButtonHeld(MouseButton::LEFT) && !selectedEntities.empty()) {
	//		const auto grabDifference = cursorPos - axisGrabStartPos;
	//		if (selectedGizmo == GizmoType::ROTATION) {
	//			Vec2 center{ 0.0f };
	//			for (const auto& pos : selectedEntitesGrabStartPositions) {
	//				center += pos;
	//			}
	//			center /= static_cast<float>(selectedEntitesGrabStartPositions.size());

	//			// Assumes the center doesn't get translated during a rotation.
	//			const auto centerToOldPos = (axisGrabStartPos - center).normalized();
	//			const auto centerToNewPos = (cursorPos - center).normalized();
	//			Debug::drawRay(center, centerToOldPos);
	//			Debug::drawRay(center, centerToNewPos);
	//			if (centerToNewPos.length() > 0.05f && centerToOldPos.length() > 0.05f) {
	//				auto angleDifference = atan2(centerToNewPos.y, centerToNewPos.x) - atan2(centerToOldPos.y, centerToOldPos.x);
	//				dbg(angleDifference);
	//				//dbg(angleDifference);
	//				//angleDifference = acos(dot(centerToOldPos.normalized(), centerToNewPos.normalized()));
	//				//angleDifference = grabDifference.x;

	//				ASSERT(selectedEntities.size() == selectedEntitesGrabStartPositions.size());
	//				for (usize i = 0; i < selectedEntities.size(); i++) {
	//					/*const auto transform =
	//						Mat3x2::translate(-selectedEntitiesCenterPos) *
	//						Mat3x2::rotate(angleDifference) *
	//						Mat3x2::translate(selectedEntitiesCenterPos);*/

	//					Vec2 p = selectedEntitesGrabStartPositions[i];
	//					p -= center;
	//					p *= Mat2::rotate(angleDifference);
	//					p += center;
	//					setEntityPos(selectedEntities[i], p);
	//					/*setEntityPos(selectedEntities[i], selectedEntitesGrabStartPositions[i] * transform);*/
	//				}

	//			}
	//				//selectedEntitesCenterPos
	//		} else if (selectedGizmo != GizmoType::NONE) {
	//			ASSERT(selectedGizmo != GizmoType::ROTATION);
	//			Vec2 translation{ 0.0f };
	//			if (selectedGizmo == GizmoType::X)
	//				translation = xAxisNormalized * xAxisLineSegment.line.distanceAlong(grabDifference);
	//			else if (selectedGizmo == GizmoType::Y)
	//				translation = yAxisNormalized * yAxisLineSegment.line.distanceAlong(grabDifference);
	//			else if (selectedGizmo == GizmoType::BOTH)
	//				translation = grabDifference;
	//			
	//			ASSERT(selectedEntities.size() == selectedEntitesGrabStartPositions.size());
	//			for (usize i = 0; i < selectedEntities.size(); i++) {
	//				setEntityPos(selectedEntities[i], selectedEntitesGrabStartPositions[i] + translation);
	//			}
	//		}
	//	}

	//	return gizmoSelected;
	//};

	//auto drawTranslationGizmo = [&, this]() -> void {
	//	if (!selectedEntities.empty()) {
	//		const auto v0 = xAxis * AXIS_BOTH_LENGTH_SCALE, v1 = yAxis * AXIS_BOTH_LENGTH_SCALE;
	//		const auto BLUE = Vec3{ 171.0f, 218.0f, 255.0f } / 255.0f;
	//		Debug::drawRay(selectedEntitiesCenterPos + v0, v1, BLUE);
	//		Debug::drawRay(selectedEntitiesCenterPos + v1, v0, BLUE);

	//		Debug::drawRay(selectedEntitiesCenterPos, xAxis, Vec3::RED);
	//		Debug::drawRay(selectedEntitiesCenterPos, yAxis, Vec3::GREEN);

	//		Debug::drawHollowCircle(selectedEntitiesCenterPos, rotationGizmoRadius, Vec3::WHITE / 2.0f);
	//	}
	//};

	const auto translationGizmoSelected = selectedEntityGizmo.update(entites, camera, selectedEntities, selectedEntitiesCenterPos, getCursorPos());

	const auto cursorPos = getCursorPos();
	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		if (!translationGizmoSelected) {
			selectedEntities.clear();
			for (usize i = 0; i < entites.entitesBody.size(); i++) {
				const auto& body = entites.entitesBody[i];
				if (contains(cursorPos, body.pos, body.orientation, body.collider)) {
					selectedEntities.push_back(Entity{ .type = EntityType::Body, .index = i });
					break;
				}
			}
		}
	}

	;

	if (Input::isMouseButtonDown(MouseButton::MIDDLE)) {
		screenGrabStartPos = cursorPos;
	} else {
		if (Input::isMouseButtonHeld(MouseButton::MIDDLE)) {
			camera.pos -= (cursorPos - screenGrabStartPos);
		}
	}

	if (Input::isKeyHeld(Keycode::CTRL) && Input::isKeyDown(Keycode::A)) {
		selectedEntities.clear();
		for (usize i = 0; i < entites.entitesBody.size(); i++) {
			selectedEntities.push_back(Entity{ .type = EntityType::Body, .index = i });
		}
	}

	updateSelectedEntitesCenterPos();

	if (const auto scroll = Input::scrollDelta(); scroll != 0.0f) {
		const auto cursorPosBeforeScroll = getCursorPos();
		const auto scrollSpeed = 15.0f * abs(scroll);
		const auto scrollIncrement = pow(scrollSpeed, Time::deltaTime());
		if (scroll > 0.0f) camera.zoom *= scrollIncrement;
		else camera.zoom /= scrollIncrement;

		camera.pos -= (getCursorPos() - cursorPosBeforeScroll);
	}

	for (const auto& body : entites.entitesBody)
		Debug::drawCollider(body.collider, body.pos, body.orientation, Vec3::WHITE);

	if (!selectedEntities.empty()) {
		selectedEntityGizmo.draw(selectedEntitiesCenterPos);
	}

	renderer.update(gfx, camera);
}

auto Editor::getCursorPos() -> Vec2 {
	return camera.screenSpaceToCameraSpace(Input::cursorPos());
}

auto Editor::updateSelectedEntitesCenterPos() -> void {
	selectedEntitiesCenterPos = Vec2{ 0.0f };
	for (const auto& entity : selectedEntities)
		selectedEntitiesCenterPos += entites.getPosOrOrigin(entity);
	selectedEntitiesCenterPos /= selectedEntities.size();
}