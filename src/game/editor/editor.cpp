#include <game/editor/editor.hpp>
#include <game/debug.hpp>
#include <imgui/imgui.h>
#include <game/editor/input.hpp>
#include <game/collision/collision.hpp>
#include <engine/time.hpp>
#include <math/lineSegment.hpp>
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
		entitesBody.push_back(BodyEditor{
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
		case EntityType::Body: entitesBody[entity.index].displayGui(); break;
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

	static constexpr float AXIS_BOTH_LENGTH_SCALE = 1.0f / 4.0f;
	auto translationGizmo = [&, this]() -> bool {
		Vec2 
			xAxisNormalized = xAxis.normalized(),
			yAxisNormalized = yAxis.normalized();
		LineSegment 
			xAxisLineSegment{ selectedEntitesCenterPos, selectedEntitesCenterPos + xAxis },
			yAxisLineSegment{ selectedEntitesCenterPos, selectedEntitesCenterPos + yAxis };

		auto translationGizmoSelected = false;
		const auto cursorPos = getCursorPos();
		if (Input::isMouseButtonDown(MouseButton::LEFT)) {
			if (!selectedEntities.empty()) {
				static constexpr auto LINE_WIDTH = 0.2f;
				selectedAxis = SelectedAxis::NONE;
				if (contains(
					cursorPos,
					selectedEntitesCenterPos + xAxis * AXIS_BOTH_LENGTH_SCALE / 2.0f + yAxis * AXIS_BOTH_LENGTH_SCALE / 2.0f,
					0.0f,
					BoxCollider{ BoxColliderEditor{.size = Vec2{ xAxis.length(), yAxis.length() } *AXIS_BOTH_LENGTH_SCALE } })) {
					selectedAxis = SelectedAxis::BOTH;
				} else if (xAxisLineSegment.asBoxContains(LINE_WIDTH, cursorPos)) {
					selectedAxis = SelectedAxis::X;
				} else if (yAxisLineSegment.asBoxContains(LINE_WIDTH, cursorPos)) {
					selectedAxis = SelectedAxis::Y;
				}

				if (selectedAxis != SelectedAxis::NONE) {
					axisGrabStartPos = cursorPos;
					selectedEntitesGrabStartPositions.clear();
					for (const auto& entity : selectedEntities) {
						selectedEntitesGrabStartPositions.push_back(getEntityPosOrOrigin(entity));
					}
					translationGizmoSelected = true;
				}
			}
		}

		if (Input::isMouseButtonHeld(MouseButton::LEFT) && !selectedEntities.empty()) {
			const auto grabDifference = cursorPos - axisGrabStartPos;
			if (selectedAxis != SelectedAxis::NONE) {
				Vec2 translation;
				if (selectedAxis == SelectedAxis::X)
					translation = xAxisNormalized * xAxisLineSegment.line.distanceAlong(grabDifference);
				else if (selectedAxis == SelectedAxis::Y)
					translation = yAxisNormalized * yAxisLineSegment.line.distanceAlong(grabDifference);
				else if (selectedAxis == SelectedAxis::BOTH)
					translation = grabDifference;

				ASSERT(selectedEntities.size() == selectedEntitesGrabStartPositions.size());
				for (usize i = 0; i < selectedEntities.size(); i++) {
					setEntityPos(selectedEntities[i], selectedEntitesGrabStartPositions[i] + translation);
				}
			}
		}
		return translationGizmoSelected;
	};

	auto drawTranslationGizmo = [&, this]() -> void {
		if (!selectedEntities.empty()) {
			const auto v0 = xAxis * AXIS_BOTH_LENGTH_SCALE, v1 = yAxis * AXIS_BOTH_LENGTH_SCALE;
			const auto BLUE = Vec3{ 171.0f, 218.0f, 255.0f } / 255.0f;
			Debug::drawRay(selectedEntitesCenterPos + v0, v1, BLUE);
			Debug::drawRay(selectedEntitesCenterPos + v1, v0, BLUE);

			Debug::drawRay(selectedEntitesCenterPos, xAxis, Vec3::RED);
			Debug::drawRay(selectedEntitesCenterPos, yAxis, Vec3::GREEN);
		}
	};

	const auto translationGizmoSelected = translationGizmo();

	const auto cursorPos = getCursorPos();
	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		if (!translationGizmoSelected) {
			selectedEntities.clear();
			for (usize i = 0; i < entitesBody.size(); i++) {
				const auto& body = entitesBody[i];
				if (contains(cursorPos, body.pos, body.orientation, body.collider)) {
					selectedEntities.push_back(Entity{ .type = EntityType::Body, .index = i });
					break;
				}
			}
		}
	}

	if (Input::isMouseButtonUp(MouseButton::LEFT)) {
		selectedAxis = SelectedAxis::NONE;
	}

	if (Input::isMouseButtonDown(MouseButton::MIDDLE)) {
		screenGrabStartPos = cursorPos;
	} else {
		if (Input::isMouseButtonHeld(MouseButton::MIDDLE)) {
			camera.pos -= (cursorPos - screenGrabStartPos);
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

	for (const auto& body : entitesBody)
		Debug::drawCollider(body.collider, body.pos, body.orientation, Vec3::WHITE);

	drawTranslationGizmo();

	renderer.update(gfx, camera);
}

auto Editor::getCursorPos() -> Vec2 {
	return camera.screenSpaceToCameraSpace(Input::cursorPos());
}

auto Editor::setEntityPos(const Entity& entity, Vec2 pos) -> void {
	switch (entity.type) {
	case EntityType::Body: entitesBody[entity.index].pos = pos; break;
	case EntityType::Null: break;
	}
}

auto Editor::getEntityPosOrOrigin(const Entity& entity) -> Vec2& {
	static Vec2 null{ 0.0f };
	switch (entity.type) {
	case EntityType::Body: return entitesBody[entity.index].pos;
	default:
		null = Vec2{ 0.0f };
		return null;
	}
}

auto Editor::updateSelectedEntitesCenterPos() -> void {
	selectedEntitesCenterPos = Vec2{ 0.0f };
	for (const auto& entity : selectedEntities)
		selectedEntitesCenterPos += getEntityPosOrOrigin(entity);
}

auto Editor::Entity::isNull() const -> bool {
	return type == EntityType::Null;
}

auto Editor::Entity::null() -> Entity {
	return Entity{ .type = EntityType::Null };
}
