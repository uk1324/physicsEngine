#include <game/editor/editorEntity.hpp>

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

auto EditorEntities::getFieldPointer(const Entity& entity, usize fieldOffset) -> u8* {
	u8* ptr;

	switch (entity.type) {
	case EntityType::Body: ptr = reinterpret_cast<u8*>(&entitesBody[entity.index]);
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
	case EntityType::Body: entitesBody[entity.index].pos = pos; break;
	case EntityType::Null: break;
	}
}

auto EditorEntities::getPosOrOrigin(const Entity& entity) -> Vec2& {
	static Vec2 null{ 0.0f };
	switch (entity.type) {
	case EntityType::Body: return entitesBody[entity.index].pos;
	default:
		null = Vec2{ 0.0f };
		return null;
	}
}

auto EditorEntities::setOrientation(const Entity& entity, float orientation) -> void {
	switch (entity.type) {
	case EntityType::Body: entitesBody[entity.index].orientation = orientation; break;
	case EntityType::Null: break;
	}
}

auto EditorEntities::getOrientationOrZero(const Entity& entity) -> float {
	static float null{ 0.0f };
	switch (entity.type) {
	case EntityType::Body: return entitesBody[entity.index].orientation;
	default:
		null = 0.0f;
		return null;
	}
}