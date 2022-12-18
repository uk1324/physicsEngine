#pragma once

#include <utils/int.hpp>
#include <game/entitesData.hpp>

#include <optional>

enum class EntityType : u8 {
	Body,
	Null,
};

struct Entity {
	EntityType type;
	u64 index;

	auto operator==(const Entity& entity) const -> bool = default;
	auto operator>(const Entity& entity) const -> bool;
	auto isNull() const -> bool;

	static auto null() -> Entity;
};

struct EditorEntities {
	// TODO: Iterator that iterates only the alive entites.
	std::vector<BodyEditor> entitesBody;
	// At some point a garbage collector like step should happen because there shouldn't be references to entites which don't exist. This would require also storing bitset for checking if an entity was already traversed.

	// Making an undoable and redoable delete would require preserving objects which got deleted. This could be done by creating a copy, but this would require a dynamic allocation when it could be using the preexisting entity pools for storing them.
	//std::vector<bool> isEnabledBody;

	// When a entites is deleted and there are references to it should either not be allowed or the references need to be removed. When undoing the remove the references should be brought back. This would either need to be a unique command for undoing a modification of a field or there would need to be some dynamic way to do it. Could store a list of strings, pointer offset pairs that would be used for doing this dynamic way. This exact thing wouldn't work for vectors, but something similar could be implemented. For vectors of vectors another thing would need to be made and so on.
	// The gui methods would need to record if a change was made and to what field and what was the previous value.

	// For generating the string pointer pairs just use offsetof

	// Another way to deal with the issue of undoing and redoing would be to store the entire state or replay the actions from the start to a certain point, but I don't think these are the right options for this.

	/*
	* Ignore this. This wasn't even a problem in the first place the problem is which entites can be deleted. No need to use versions just use the isEnabledFlag for displaying messages about invalid references.
	// Another way to deal with invalid references is to just ignore them. And store a version of the entity somewhere so the version can be check to checked if the entity is valid. This is how it will work at runtime. 
	*/

	// Relative pointers?
	// Aren't entites just relative pointers?

	// Levels should be check when loaded if the references are valid.

	// https://gamedev.stackexchange.com/questions/101964/implementing-the-command-pattern-undo-and-entity-references

	auto getFieldPointer(const Entity& entity, usize fieldOffset) -> u8*;

	auto setPos(const Entity& entity, Vec2 pos) -> void;
	auto getPosOrOrigin(const Entity& entity) -> Vec2&;
	auto getPos(const Entity& entity) -> std::optional<Vec2>;

	auto setOrientation(const Entity& entity, float orientation) -> void;
	auto getOrientationOrZero(const Entity& entity) -> float;

	auto getAabb(const Entity& entity) -> std::optional<Aabb>;
};