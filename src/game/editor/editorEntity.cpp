#include <game/editor/editorEntity.hpp>

auto Entity::isNull() const -> bool {
	return type == EntityType::Null;
}

auto Entity::null() -> Entity {
	return Entity{ .type = EntityType::Null };
}

auto EditorEntites::setPos(const Entity& entity, Vec2 pos) -> void {
	switch (entity.type) {
	case EntityType::Body: entitesBody[entity.index].pos = pos; break;
	case EntityType::Null: break;
	}
}

auto EditorEntites::getPosOrOrigin(const Entity& entity) -> Vec2& {
	static Vec2 null{ 0.0f };
	switch (entity.type) {
	case EntityType::Body: return entitesBody[entity.index].pos;
	default:
		null = Vec2{ 0.0f };
		return null;
	}
}

auto EditorEntites::getOrientationOrZero(const Entity& entity) -> float {
	static float null{ 0.0f };
	switch (entity.type) {
	case EntityType::Body: return entitesBody[entity.index].orientation;
	default:
		null = 0.0f;
		return null;
	}
}