#include <game/editor/distanceJointGizmo.hpp>
#include <game/editor/input.hpp>
#include <game/editor/commands.hpp>
#include <game/debug.hpp>
#include <math/mat2.hpp>

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
	const auto& bodyB = entites.body[distanceJoint.anchorB.body];

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
		checkEntityAnchorGrab(distanceJoint.anchorB, Anchor::B);
	}

	if (Input::isMouseButtonUp(MouseButton::LEFT)) {
		const auto entity = Entity{ .type = EntityType::DistanceJoint, .index = *distanceJointEntityIndex };
		auto saveAnchorUpdateChanges = [this, &entity, &commands](const Body& body, Vec2 newOffset, usize ptrOffset) -> void {
			const auto oldOffset = (anchorGrabStartPos - body.pos) * Mat2::rotate(-body.orientation);
			commands.addSetFieldCommand(entity, DISTANCE_JOINT_ANCHOR_EDITOR_OBJECT_SPACE_OFFSET_OFFSET + ptrOffset, &oldOffset, &newOffset, sizeof(anchorGrabStartPos));
		};

		if (grabbedAnchor == Anchor::A) {
			saveAnchorUpdateChanges(bodyA, distanceJoint.anchorA.objectSpaceOffset, DISTANCE_JOINT_ENTITY_EDITOR_ANCHOR_A_OFFSET);
		} else if (grabbedAnchor == Anchor::B) {
			saveAnchorUpdateChanges(bodyB, distanceJoint.anchorB.objectSpaceOffset, DISTANCE_JOINT_ENTITY_EDITOR_ANCHOR_B_OFFSET);
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

	const auto& bodyA = entites.body[distanceJoint.anchorA.body];
	const auto& bodyB = entites.body[distanceJoint.anchorB.body];
	const auto posA = bodyA.pos + distanceJoint.anchorA.objectSpaceOffset * Mat2::rotate(bodyA.orientation);
	const auto posB = bodyB.pos + distanceJoint.anchorB.objectSpaceOffset * Mat2::rotate(bodyB.orientation);

	const auto UNSELECTED_RED = Vec3::RED * 2.0f / 3.0f;
	Debug::drawPoint(posA, grabbedAnchor == Anchor::A ? Vec3::RED : UNSELECTED_RED);
	Debug::drawPoint(posB, grabbedAnchor == Anchor::B ? Vec3::RED : UNSELECTED_RED);
	Debug::drawLine(bodyA.pos, posA, Vec3::GREEN);
	Debug::drawLine(bodyA.pos, posA, Vec3::GREEN);
}

auto DistanceJointGizmo::displayGizmo(const std::vector<Entity>& selectedEntities) -> std::optional<usize> {
	if (selectedEntities.size() == 1 && selectedEntities[0].type == EntityType::DistanceJoint) {
		return selectedEntities[0].index;
	}
	return std::nullopt;
}
