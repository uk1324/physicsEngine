#pragma once

#include <game/editor/editorEntity.hpp>
#include <game/camera.hpp>

struct DistanceJointGizmo {
	auto update(
		const std::vector<Entity>& selectedEntites,
		EditorEntities& entites,
		Vec2 cursorPos,
		const Camera& camera,
		Commands& commands) -> bool;
	auto draw(const std::vector<Entity> selectedEntites, const EditorEntities& entites) -> void;

	enum class Anchor {
		A,
		B,
		NONE
	};
	Anchor grabbedAnchor = Anchor::NONE;
	Vec2 anchorGrabStartPos{ 0.0f };

	static auto displayGizmo(const std::vector<Entity>& selectedEntities)->std::optional<usize>;
};