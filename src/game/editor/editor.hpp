#pragma once

#include <game/entitesData.hpp>
#include <game/renderer.hpp>

class Editor {
public:
	Editor();
	auto update(Gfx& gfx, Renderer& renderer) -> void;

private:

	// Cycling selecting can't just be implemented by comparing with the slected item because this would only work if there were 2 overlapping entites.

	auto getCursorPos() -> Vec2;

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
	auto setEntityPos(const Entity& entity, Vec2 pos) -> void;
	auto getEntityPosOrOrigin(const Entity& entity) -> Vec2&;
	
	// Maybe make this a single struct.
	std::vector<Entity> selectedEntities;
	auto updateSelectedEntitesCenterPos() -> void;
	Vec2 selectedEntitesCenterPos;

	Vec2 screenGrabStartPos;
	Vec2 centerGrabStartPos;

	enum class SelectedAxis {
		X, Y, BOTH, NONE
	};
	SelectedAxis selectedAxis = SelectedAxis::NONE;
	Vec2 axisGrabStartPos;
	std::vector<Vec2> selectedEntitesGrabStartPositions;

	std::vector<BodyEditor> entitesBody;

	Camera camera;
};