#pragma once

#include <utils/int.hpp>
#include <game/entitesData.hpp>

enum class EntityType {
	Body,
	Null,
};

struct Entity {
	EntityType type;
	u64 index;

	auto isNull() const -> bool;

	static auto null() -> Entity;
};

struct EditorEntites {
	std::vector<BodyEditor> entitesBody;

	auto setPos(const Entity& entity, Vec2 pos) -> void;
	auto getPosOrOrigin(const Entity& entity) -> Vec2&;
	auto getOrientationOrZero(const Entity& entity) -> float;
};