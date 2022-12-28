#pragma once

#include <game/editor/editorEntity.hpp>
//#include <utils/refOptional.hpp>

struct ScalingGizmo {
	auto update(const std::vector<Entity> selectedEntities, Commands& commands, Vec2 cursorPos, EditorEntities& entites, float pointRadius) -> bool;
	auto draw(const std::vector<Entity> selectedEntities, EditorEntities& entities) const -> void;

private:
	struct Result {
		bool grabStarted;
		bool isGrabbing;
	};

	auto boxScaling(BoxCollider& scaledBox, bool uniformScaling, Vec2 cursorPos, float pointRadius) -> Result;

	struct Box {
		BodyEditor& body;
		BoxCollider& box;
		usize bodyIndex;
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
	
};