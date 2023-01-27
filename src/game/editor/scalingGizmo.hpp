#pragma once

#include <game/editor/editorEntity.hpp>
//#include <utils/refOptional.hpp>

struct ScalingGizmo {
	auto update(const std::vector<Entity> selectedEntities, const Aabb& selectedEntitiesAabb, Commands& commands, Vec2 cursorPos, EditorEntities& entites, float pointRadius) -> bool;
	auto draw(const std::vector<Entity> selectedEntities, const Aabb& selectedEntitiesAabb, EditorEntities& entities) const -> void;

private:
	struct Result {
		bool grabStarted;
		bool isGrabbing;
		bool releasedGrab;
		Vec2 signedScale;
		Vec2 offset;
	};

	auto boxScaling(BoxCollider& box, Vec2& boxPos, float boxOrientation, bool uniformScaling, Vec2 cursorPos, float pointRadius, bool selectedEntitiesChanges) -> Result;

	struct Box {
		const BodyOldEditor& body;
		const BoxCollider& box;
	};
	static auto isOnlyBoxSelected(const std::vector<Entity> selectedEntities, EditorEntities& entites) -> std::optional<Box>;
	static auto isEdge(usize feature) -> bool;

	enum BoxFeature {
		CORNER_0,
		CORNER_1,
		CORNER_2,
		CORNER_3,
		EDGE_0,
		EDGE_1,
		EDGE_2,
		EDGE_3,
	};
	std::optional<usize> grabbedFeature;
	Vec2 boxGrabStartBodyPos{ 0.0f };
	Vec2 boxGrabStartPos{ 0.0f };
	BoxColliderEditor boxGrabStartCollider{ Vec2{ 0.0f } };

	std::vector<Vec2> selectedEntitesBoxGrabStartPositions;
	std::vector<Collider> selectedEntitesBoxGrabStartColliders;
	std::vector<Entity> grabStartSelectedEntites;
};