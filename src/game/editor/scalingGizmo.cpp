#include <game/editor/scalingGizmo.hpp>
#include <game/editor/input.hpp>
#include <game/editor/commands.hpp>
#include <math/mat2.hpp>
#include <math/vec3.hpp>
#include <math/utils.hpp>
#include <engine/debug.hpp>

#include <utils/overloaded.hpp>

auto ScalingGizmo::update(const std::vector<Entity> selectedEntities, const Aabb& selectedEntitiesAabb, Commands& commands, Vec2 cursorPos, EditorEntities& entites, float pointRadius) -> bool {
	auto isGrabbing = false;

	if (selectedEntities.size() == 0)
		return isGrabbing;

	auto box = BoxCollider{ BoxColliderEditor{.size = selectedEntitiesAabb.size() } };
	auto pos = selectedEntitiesAabb.center();
	float orientation = 0.0f;
	auto doUniformScaling = true;

	if (const auto& optionalBox = isOnlyBoxSelected(selectedEntities, entites)) {
		const auto& [body, boxCollider] = *optionalBox;
		doUniformScaling = false;
		orientation = body.orientation;
		pos = body.pos;
		box = boxCollider;
	}

	const auto selectedEntitiesChanged = selectedEntities != grabStartSelectedEntites;

	const auto result = boxScaling(box, pos, orientation, doUniformScaling, cursorPos, pointRadius, selectedEntitiesChanged);
	isGrabbing = result.isGrabbing;

	if (result.releasedGrab || (isGrabbing && selectedEntitiesChanged)) {
		ASSERT(selectedEntitesBoxGrabStartPositions.size() == grabStartSelectedEntites.size());
		ASSERT(selectedEntitesBoxGrabStartColliders.size() == grabStartSelectedEntites.size());
		commands.beginMulticommand();
		for (usize i = 0; i < grabStartSelectedEntites.size(); i++) {
			const auto& entity = grabStartSelectedEntites[i];
			if (entity.type == EntityType::Body) {
				auto& body = entites.body[entity.index];
				commands.addSetFieldCommand(entity, BODY_EDITOR_POS_OFFSET, selectedEntitesBoxGrabStartPositions[i], body.pos);
				commands.addSetFieldCommand(entity, BODY_EDITOR_COLLIDER_OFFSET, selectedEntitesBoxGrabStartColliders[i], body.collider);
			}
		}
		commands.endMulticommand();
	}

	if (result.grabStarted) {
		grabStartSelectedEntites = selectedEntities;
		selectedEntitesBoxGrabStartPositions.clear();
		selectedEntitesBoxGrabStartColliders.clear();
		for (const auto& entity : grabStartSelectedEntites) {
			if (entity.type == EntityType::Body) {
				const auto& body = entites.body[entity.index];
				selectedEntitesBoxGrabStartPositions.push_back(body.pos);
				selectedEntitesBoxGrabStartColliders.push_back(body.collider);
			} else {
				selectedEntitesBoxGrabStartPositions.push_back(Vec2{ 0.0f });
				selectedEntitesBoxGrabStartColliders.push_back(CircleColliderEditor{ 0.0f });
			}
		}
	}

	if (isGrabbing && selectedEntitesBoxGrabStartPositions.size() == grabStartSelectedEntites.size()) {
		ASSERT(selectedEntitesBoxGrabStartColliders.size() == grabStartSelectedEntites.size());
		for (usize i = 0; i < grabStartSelectedEntites.size(); i++) {
			const auto& entity = grabStartSelectedEntites[i];
			if (entity.type == EntityType::Body) {
				auto& body = entites.body[entity.index];
				body.pos = selectedEntitesBoxGrabStartPositions[i].scaledAround(boxGrabStartBodyPos, result.signedScale) + result.offset;
				body.collider = std::visit(overloaded{
					[&](BoxCollider box) -> Collider {
						box.size *= result.signedScale.applied(abs);
						return box;
					},
					[&](CircleCollider circle) -> Collider {
						circle.radius *= abs(result.signedScale.x);
						return circle;
					}
					}, selectedEntitesBoxGrabStartColliders[i]);
			}
		}
	}

	return isGrabbing;
}

auto ScalingGizmo::draw(const std::vector<Entity> selectedEntities, const Aabb& selectedEntitiesAabb, EditorEntities& entities) const -> void {
	std::array<Vec2, 4> corners;
	if (auto value = isOnlyBoxSelected(selectedEntities, entities)) {
		const auto& [body, box] = *value;
		corners = box.getCorners(body.pos, body.orientation);
	} else {
		corners = selectedEntitiesAabb.getCorners();
		Debug::drawAabb(selectedEntitiesAabb);
	}

	const auto UNSELECTED_RED = Vec3::RED * 2.0f / 3.0f;
	for (usize i = 0; i < corners.size(); i++) {
		Debug::drawPoint(corners[i], grabbedFeature == i ? Vec3::RED : UNSELECTED_RED);
	}
}

auto ScalingGizmo::boxScaling(BoxCollider& box, Vec2& boxPos, float boxOrientation, bool uniformScaling, Vec2 cursorPos, float pointRadius, bool selectedEntitiesChanged) -> Result {
	auto grabbingStarted = false;

	// TODO: Uniform scaling and centered scaling.
	if (Input::isMouseButtonDown(MouseButton::LEFT) || (selectedEntitiesChanged && grabbedFeature.has_value())) {
		const auto edges = box.getEdges(boxPos, boxOrientation);
		for (usize i = 0; i < edges.size(); i++) {
			if (edges[i].asCapsuleContains(pointRadius, cursorPos)) {
				grabbedFeature = EDGE_0 + i;
				boxGrabStartPos = edges[i].line.projectPointOntoLine(cursorPos);
			}
		}

		const auto corners = box.getCorners(boxPos, boxOrientation);
		for (usize i = 0; i < corners.size(); i++) {
			if (distance(corners[i], cursorPos) < pointRadius) {
				grabbedFeature = CORNER_0 + i;
				boxGrabStartPos = corners[i];
			}
		}

		if (grabbedFeature.has_value()) {
			grabbingStarted = true;
			boxGrabStartBodyPos = boxPos;
			boxGrabStartCollider = box;
		}
	}

	auto isGrabbing = false;
	Vec2 scale{ 1.0f };
	Vec2 translation{ 0.0f };
	if (grabbedFeature.has_value()) {
		isGrabbing = true;

		const auto& axes = Mat2::rotate(boxOrientation);

		Vec2 offset = (cursorPos - boxGrabStartPos) * axes.orthonormalInv();

		if (isEdge(*grabbedFeature)) {
			if (*grabbedFeature % 2 == 0) {
				offset.x = 0.0f;
			} else {
				offset.y = 0.0f;
			}
		}

		const auto corners = BoxCollider{ boxGrabStartCollider }.getCorners(boxGrabStartBodyPos, boxOrientation);
		const auto fixedCorner = (*grabbedFeature + 2) % 4;
		const auto ratio = boxGrabStartCollider.size.x / boxGrabStartCollider.size.y;

		const auto direction = ((boxGrabStartPos - boxGrabStartBodyPos) * Mat2::rotate(-boxOrientation)).applied(sign);
		box.size = boxGrabStartCollider.size + offset * direction;

		// This only accounts for size changes done in the y direction. When doing checks which change is bigger (abs(offset.x) < abs(offset.y)) weird snapping appears so I decided to just keep this version.
		// Gimp also only allows scaling in on axis like in this. Unity accounts for which change is bigger, but it doesn't allow mirroring. Not sure if mirroring wasn't allowed the snapping glitches would still appear.
		if (uniformScaling) {
			box.size.x = abs(box.size.y) * ratio * sign(box.size.x);
		}

		scale = box.size / boxGrabStartCollider.size;

		const auto newCorners = box.getCorners(boxGrabStartBodyPos, boxOrientation);
		const auto fixedCornerOffset = corners[fixedCorner] - newCorners[fixedCorner];
		translation = fixedCornerOffset;
		boxPos = boxGrabStartBodyPos + fixedCornerOffset;

		box.size = box.size.applied(abs);
	}

	auto releasedGrab = false;
	if (grabbedFeature.has_value() && Input::isMouseButtonUp(MouseButton::LEFT)) {
		grabbedFeature = std::nullopt;
		releasedGrab = true;
	}

	return Result{ grabbingStarted, isGrabbing, releasedGrab, scale, translation };
}

auto ScalingGizmo::isOnlyBoxSelected(const std::vector<Entity> selectedEntities, EditorEntities& entites) -> std::optional<Box> {
	if (selectedEntities.size() == 1 && selectedEntities[0].type == EntityType::Body) {
		auto& body = entites.body[selectedEntities[0].index];
		if (auto box = std::get_if<BoxCollider>(&body.collider)) {
			return Box{ body, *box };
		}
	}
	return std::nullopt;
}

auto ScalingGizmo::isEdge(usize feature) -> bool {
	return feature >= ScalingGizmo::EDGE_0 && feature <= ScalingGizmo::EDGE_3;
}
