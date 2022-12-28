#include <game/editor/scalingGizmo.hpp>
#include <game/editor/input.hpp>
#include <game/editor/commands.hpp>
#include <math/mat2.hpp>
#include <math/vec3.hpp>
#include <math/utils.hpp>
#include <game/debug.hpp>

auto ScalingGizmo::update(const std::vector<Entity> selectedEntities, Commands& commands, Vec2 cursorPos, EditorEntities& entites, float pointRadius) -> bool {
	bool grabbing = false;
	if (auto value = isOnlyBoxSelected(selectedEntities, entites)) {
		const auto& [body, box, bodyIndex] = *value;
		// TODO: Uniform scaling and centered scaling.
		if (Input::isMouseButtonDown(MouseButton::LEFT)) {
			const auto edges = box.getEdges(body.pos, body.orientation);
			for (usize i = 0; i < edges.size(); i++) {
				if (edges[i].asCapsuleContains(pointRadius, cursorPos)) {
					grabbedFeature = EDGE_0 + i;
				}
			}

			const auto corners = box.getCorners(body.pos, body.orientation);
			for (usize i = 0; i < corners.size(); i++) {
				if (distance(corners[i], cursorPos) < pointRadius) {
					grabbedFeature = CORNER_0 + i;
				}
			}

			if (grabbedFeature.has_value()) {
				boxGrabStartBodyPos = body.pos;
				boxGrabStartCollider = box;
				boxGrabStartPos = cursorPos;
			}
		}

		if (grabbedFeature.has_value()) {
			grabbing = true;

			const auto direction = ((boxGrabStartPos - boxGrabStartBodyPos) * Mat2::rotate(-body.orientation)).applied(sign);
			const auto offset = (cursorPos - boxGrabStartPos);
			const auto& axes = Mat2::rotate(body.orientation);

			const Vec2 offsetProjected = offset * axes.orthonormalInv();

			auto positionDiference = offset / 2.0f;
			auto sizeDiference = offsetProjected * direction;
			if (isEdge(*grabbedFeature)) {
				// Could dot and then multiple by the doted vector instead.
				positionDiference *= axes.orthonormalInv();
				if (*grabbedFeature % 2 == 0) {
					sizeDiference.x = 0.0f;
					positionDiference.x = 0.0f;
				} else {
					sizeDiference.y = 0.0f;
					positionDiference.y = 0.0f;
				}
				positionDiference *= axes;
			}

			body.pos = boxGrabStartBodyPos + positionDiference;
			box.size = boxGrabStartCollider.size + sizeDiference;
			box.size = box.size.applied(abs);
		}

		if (grabbedFeature.has_value() && Input::isMouseButtonUp(MouseButton::LEFT)) {
			grabbedFeature = std::nullopt;
			commands.beginMulticommand();
			const Entity entity{ .type = EntityType::Body, .index = bodyIndex };
			commands.addSetFieldCommand(entity, BODY_EDITOR_POS_OFFSET, boxGrabStartBodyPos, body.pos);
			const Collider oldCollider{ boxGrabStartCollider };
			commands.addSetFieldCommand(entity, BODY_EDITOR_COLLIDER_OFFSET, oldCollider, body.collider);
			commands.endMulticommand();
		}
	}

	return grabbing;
}

auto ScalingGizmo::draw(const std::vector<Entity> selectedEntities, EditorEntities& entities) const -> void {
	if (auto value = isOnlyBoxSelected(selectedEntities, entities)) {
		const auto& [body, box, _] = *value;
		const auto corners = box.getCorners(body.pos, body.orientation);
		const auto UNSELECTED_RED = Vec3::RED * 2.0f / 3.0f;
		for (usize i = 0; i < corners.size(); i++) {
			Debug::drawPoint(corners[i], grabbedFeature == i ? Vec3::RED : UNSELECTED_RED);
		}
	}
}

//auto ScalingGizmo::boxScaling(BoxCollider& scaledBox, bool uniformScaling, Vec2 cursorPos, float pointRadius) -> Result
//{
//	return Result();
//}

auto ScalingGizmo::isOnlyBoxSelected(const std::vector<Entity> selectedEntities, EditorEntities& entites) -> std::optional<Box> {
	if (selectedEntities.size() == 1 && selectedEntities[0].type == EntityType::Body) {
		auto& body = entites.body[selectedEntities[0].index];
		if (auto box = std::get_if<BoxCollider>(&body.collider)) {
			return Box{ body, *box, selectedEntities[0].index };
		}
	}
	return std::nullopt;
}

auto ScalingGizmo::isEdge(usize feature) -> bool {
	return feature >= ScalingGizmo::EDGE_0 && feature <= ScalingGizmo::EDGE_3;
}
