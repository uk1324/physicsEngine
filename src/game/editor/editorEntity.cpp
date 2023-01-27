#include <game/editor/editorEntity.hpp>
#include <game/collision.hpp>
#include <math/mat2.hpp>
#include <utils/overloaded.hpp>

auto Entity::operator>(const Entity& entity) const -> bool {
	return index > entity.index && (static_cast<u32>(type) == static_cast<u32>(entity.type))
		|| index > entity.index && (static_cast<u32>(type) > static_cast<u32>(entity.type));
}

auto Entity::isNull() const -> bool {
	return type == EntityType::Null;
}

auto Entity::null() -> Entity {
	return Entity{ .type = EntityType::Null };
}

auto EditorEntities::getDistanceJointEndpoints(const DistanceJointEntityEditor& joint) const -> std::pair<Vec2, Vec2> {
	const auto& bodyA = body[joint.anchorA.body];
	const auto& bodyB = body[joint.anchorB.body];
	const auto posA = bodyA.pos + joint.anchorA.objectSpaceOffset * Mat2::rotate(bodyA.orientation);
	const auto posB = bodyB.pos + joint.anchorB.objectSpaceOffset * Mat2::rotate(bodyB.orientation);
	return { posA, posB };
}

auto EditorEntities::getDistanceJointLineSegment(const DistanceJointEntityEditor& joint) const -> LineSegment {
	const auto [a, b] = getDistanceJointEndpoints(joint);
	return LineSegment{ a, b };
}

auto EditorEntities::getPosPointerOffset(const Entity& entity) -> std::optional<usize> {
	switch (entity.type) {
	case EntityType::Body: return BODY_OLD_EDITOR_POS_OFFSET;
	default: return std::nullopt;
	}
}

auto EditorEntities::getOrientationPointerOffset(const Entity& entity) -> std::optional<usize> {
	switch (entity.type) {
	case EntityType::Body: return BODY_OLD_EDITOR_ORIENTATION_OFFSET;
	default: return std::nullopt;
	}
}

auto EditorEntities::getFieldPointer(const Entity& entity, usize fieldOffset) -> u8* {
	u8* ptr;

	switch (entity.type) {
	case EntityType::Body: 
		ptr = reinterpret_cast<u8*>(&body[entity.index]);
		if (fieldOffset > sizeof(BodyOldEditor)) {
			ASSERT_NOT_REACHED();
			return nullptr;
		}
		break;
	case EntityType::DistanceJoint:
		ptr = reinterpret_cast<u8*>(&distanceJoint[entity.index]);
		if (fieldOffset > sizeof(DistanceJointEntityEditor)) {
			ASSERT_NOT_REACHED();
			return nullptr;
		}
		break;
	default:
		ASSERT_NOT_REACHED();
		return nullptr;
	}

	return ptr + fieldOffset;
}

auto EditorEntities::setPos(const Entity& entity, Vec2 pos) -> void {
	switch (entity.type) {
	case EntityType::Body: body[entity.index].pos = pos; break;
	case EntityType::DistanceJoint: // Distance joints get translated with the bodies they are attached to.
	case EntityType::Null: 
		break;
	}
}

auto EditorEntities::getPosOrOrigin(const Entity& entity) -> Vec2& {
	static Vec2 null{ 0.0f };
	switch (entity.type) {
	case EntityType::Body: return body[entity.index].pos;

	case EntityType::DistanceJoint:
	case EntityType::Null:
		return null;
	}
	ASSERT_NOT_REACHED();
	return null;
}

auto EditorEntities::getPos(const Entity& entity) -> std::optional<Vec2> {
	switch (entity.type) {
	case EntityType::Body: return body[entity.index].pos;
	case EntityType::DistanceJoint:
	case EntityType::Null:
		break;
	}
	return std::nullopt;

}

auto EditorEntities::setOrientation(const Entity& entity, float orientation) -> void {
	switch (entity.type) {
	case EntityType::Body: body[entity.index].orientation = orientation; break;
	case EntityType::DistanceJoint:
	case EntityType::Null: 
		break;
	}
}

auto EditorEntities::getOrientationOrZero(const Entity& entity) -> float {
	static float null{ 0.0f };
	switch (entity.type) {
	case EntityType::Body: return body[entity.index].orientation;
	default:
		null = 0.0f;
		return null;
	}
}

auto EditorEntities::getAabb(const Entity& entity) -> std::optional<Aabb> {
	// If the entity is just a pos the return an Aabb with min = max = pos.
	switch (entity.type) {
	case EntityType::Body: {
		const auto& bodyEntity = body[entity.index];
		return aabb(bodyEntity.collider, bodyEntity.pos, bodyEntity.orientation);
	}
	default: return std::nullopt;
	}
}

auto EditorEntities::isAlive(const Entity& entity) const -> bool {
	switch (entity.type) {
	case EntityType::Body: return body.isAlive[entity.index];
	case EntityType::DistanceJoint: return distanceJoint.isAlive[entity.index];
	case EntityType::Null: return false;
	}
	ASSERT_NOT_REACHED();
	return false;
}

auto EditorEntities::setIsAlive(const Entity& entity, bool value) -> void {
	switch (entity.type) {
	case EntityType::Body: body.isAlive[entity.index] = value; return;
	case EntityType::DistanceJoint: distanceJoint.isAlive[entity.index] = value; return;
	case EntityType::Null: return;
	}
	ASSERT_NOT_REACHED();
}
