#include <game/editor/editorEntity.hpp>
#include <game/collision/collision.hpp>

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

auto EditorEntities::getPosPointerOffset(const Entity& entity) -> std::optional<usize> {
	switch (entity.type) {
	case EntityType::Body: return BODY_EDITOR_POS_OFFSET;
	default: return std::nullopt;
	}
}

auto EditorEntities::getOrientationPointerOffset(const Entity& entity) -> std::optional<usize> {
	switch (entity.type) {
	case EntityType::Body: return BODY_EDITOR_ORIENTATION_OFFSET;
	default: return std::nullopt;
	}
}

auto EditorEntities::getFieldPointer(const Entity& entity, usize fieldOffset) -> u8* {
	u8* ptr;

	switch (entity.type) {
	case EntityType::Body: ptr = reinterpret_cast<u8*>(&body[entity.index]);
		if (fieldOffset > sizeof(BodyEditor)) {
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
	case EntityType::Null: break;
	}
}

auto EditorEntities::getPosOrOrigin(const Entity& entity) -> Vec2& {
	static Vec2 null{ 0.0f };
	switch (entity.type) {
	case EntityType::Body: return body[entity.index].pos;
	default:
		null = Vec2{ 0.0f };
		return null;
	}
}

auto EditorEntities::getPos(const Entity& entity) -> std::optional<Vec2> {
	switch (entity.type) {
	case EntityType::Body: return body[entity.index].pos;
	default:
		return std::nullopt;
	}
}

auto EditorEntities::setOrientation(const Entity& entity, float orientation) -> void {
	switch (entity.type) {
	case EntityType::Body: body[entity.index].orientation = orientation; break;
	case EntityType::Null: break;
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
	default: return std::nullopt; break;
	}
}
