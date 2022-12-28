#include <game/editor/distanceJointGizmo.hpp>
#include <game/editor/input.hpp>
#include <game/editor/commands.hpp>
#include <game/debug.hpp>
#include <math/mat2.hpp>
#include <utils/overloaded.hpp>

auto DistanceJointGizmo::update(
	const std::vector<Entity>& selectedEntites, 
	EditorEntities& entites, 
	Vec2 cursorPos, 
	const Camera& camera,
	Commands& commands) -> bool {

	const auto distanceJointEntityIndex = displayGizmo(selectedEntites);
	if (!distanceJointEntityIndex.has_value())
		return false;

	auto& distanceJoint = entites.distanceJoint[*distanceJointEntityIndex];
	const auto selectCircleRadius = 0.05f / camera.zoom;

	const auto& bodyA = entites.body[distanceJoint.anchorA.body];
	const auto anchorA = bodyA.pos + distanceJoint.anchorA.objectSpaceOffset;

	auto clickedPoint = [&selectCircleRadius, &cursorPos](Vec2 point) -> bool {
		return Input::isMouseButtonDown(MouseButton::LEFT) && distance(cursorPos, point) < selectCircleRadius;
	};

	auto isGizmoGrabbed = false;

	auto checkEntityAnchorGrab = [&](DistanceJointAnchorEditor& anchor, Anchor anchorType) -> void {
		const auto& body = entites.body[anchor.body];
		const auto anchorPos = body.pos + anchor.objectSpaceOffset * Mat2::rotate(body.orientation);
		if (clickedPoint(anchorPos)) {
			grabbedAnchor = anchorType;
			anchorGrabStartPos = anchorPos;
		}

		if (grabbedAnchor == anchorType) {
			isGizmoGrabbed = true;
			const auto old = anchor.objectSpaceOffset;
			anchor.objectSpaceOffset = (cursorPos - body.pos) * Mat2::rotate(-body.orientation);
		}
	};

	checkEntityAnchorGrab(distanceJoint.anchorA, Anchor::A);

	if (grabbedAnchor != Anchor::A) {
		std::visit(overloaded{
			[&](DistanceJointAnchorEditor& anchorB) {
				checkEntityAnchorGrab(anchorB, Anchor::B);
			},
			[&](Vec2& anchorB) {
				if (clickedPoint(anchorB)) {
					grabbedAnchor = Anchor::B;
					anchorGrabStartPos = anchorB;
				}
				if (grabbedAnchor == Anchor::B) {
					anchorB = cursorPos;
					isGizmoGrabbed = true;
				}
			},
		}, distanceJoint.staticWorldSpaceAnchorOrBodyAnchorB);
	}

	if (Input::isMouseButtonUp(MouseButton::LEFT)) {
		const auto entity = Entity{ .type = EntityType::DistanceJoint, .index = *distanceJointEntityIndex };
		if (grabbedAnchor == Anchor::A) {
			const auto oldOffset = (anchorGrabStartPos - bodyA.pos) * Mat2::rotate(-bodyA.orientation);
			commands.addSetFieldCommand(entity, DISTANCE_JOINT_ENTITY_EDITOR_ANCHOR_A_OFFSET + DISTANCE_JOINT_ANCHOR_EDITOR_OBJECT_SPACE_OFFSET_OFFSET, &oldOffset, &distanceJoint.anchorA.objectSpaceOffset, sizeof(anchorGrabStartPos));
		} else if (grabbedAnchor == Anchor::B) {
			std::visit(overloaded{
				[&](DistanceJointAnchorEditor& anchorB) {
					const auto& body = entites.body[anchorB.body];
					const auto oldOffset = (anchorGrabStartPos - body.pos) * Mat2::rotate(-body.orientation);
					const decltype(DistanceJointEntityEditor::staticWorldSpaceAnchorOrBodyAnchorB) oldValue = DistanceJointAnchorEditor{
						.body = anchorB.body,
						.objectSpaceOffset = oldOffset
					};

					commands.addSetFieldCommand(entity, DISTANCE_JOINT_ENTITY_EDITOR_STATIC_WORLD_SPACE_ANCHOR_OR_BODY_ANCHOR_B_OFFSET, &oldValue, &distanceJoint.staticWorldSpaceAnchorOrBodyAnchorB, sizeof(oldValue));
				},
				[&](Vec2& anchorB) {
					const decltype(DistanceJointEntityEditor::staticWorldSpaceAnchorOrBodyAnchorB) oldValue = anchorGrabStartPos;
					commands.addSetFieldCommand(entity, DISTANCE_JOINT_ENTITY_EDITOR_STATIC_WORLD_SPACE_ANCHOR_OR_BODY_ANCHOR_B_OFFSET, &anchorGrabStartPos, &distanceJoint.staticWorldSpaceAnchorOrBodyAnchorB, sizeof(oldValue));
				},
		}, distanceJoint.staticWorldSpaceAnchorOrBodyAnchorB);
		}
		grabbedAnchor = Anchor::NONE;
	}

	return isGizmoGrabbed;
}

auto DistanceJointGizmo::draw(const std::vector<Entity> selectedEntites, const EditorEntities& entites) -> void {
	const auto distanceJointEntity = displayGizmo(selectedEntites);
	if (!distanceJointEntity.has_value())
		return;
	const auto& distanceJoint = entites.distanceJoint[*distanceJointEntity];

	const auto [a, b] = entites.getDistanceJointEndpoints(distanceJoint);
	const auto UNSELECTED_RED = Vec3::RED * 2.0f / 3.0f;
	Debug::drawPoint(a, grabbedAnchor == Anchor::A ? Vec3::RED : UNSELECTED_RED);
	Debug::drawPoint(b, grabbedAnchor == Anchor::B ? Vec3::RED : UNSELECTED_RED);
}

auto DistanceJointGizmo::displayGizmo(const std::vector<Entity>& selectedEntities) -> std::optional<usize> {
	if (selectedEntities.size() == 1 && selectedEntities[0].type == EntityType::DistanceJoint) {
		return selectedEntities[0].index;
	}
	return std::nullopt;
}
