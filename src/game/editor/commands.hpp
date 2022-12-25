#pragma once

#include <game/editor/editorEntity.hpp>
#include <memory>

struct SetFieldCommand {
	Entity entity;
	u8 size;
	usize pointerOffset;

	// It should be possible to represent the difference between 2 states as a sequence which stores which bits were changed.
	// a - 1010110
	// b - 0010101
	// d - 1000011
	// This is just XOR. One issue with this if something was changed, but it wasn't added as a command then it might not get undone correctly.
	// @Performance: Could store the data appended to this struct so the pointer doesn't need to be stored. This would require storing the usize ptr in the variant.
	usize oldDataPtr;
	usize newDataPtr;
};

struct SelectCommand {
	std::vector<Entity> oldSelectedEntites;
	std::vector<Entity> newSelectedEntites;
};

struct CreateEntityCommand {
	Entity entity;
};

struct DeleteEntityCommand {
	Entity entity;
};

using Command = std::variant<SetFieldCommand, SelectCommand, CreateEntityCommand, DeleteEntityCommand>;

// @Performance: For storing variable sized types could store pointers to data allocated on the command allocator stack.
struct Commands {
	std::vector<Command> commandStack;
	// Could rename to command range sizes or crate a struct to make this more readable.
	std::vector<usize> commandSizes;
	usize commandsSizesTop = 0; // 1 above the current command.

	// Could use RAII for this instead.
	auto beginMulticommand() -> void;
	auto endMulticommand() -> void;

	auto addCommand(Command&& command) noexcept -> void;
	auto addSetFieldCommand(const Entity& entity, usize pointerOffset, const void* oldValue, const void* newValue, u8 size) -> void;
	auto addSelectCommand(Span<const Entity> oldSelectedEntites, Span<const Entity> newSelectedEntites) -> void;

	auto getPtr(usize ptr) -> u8*;
private:
	bool recordingMulticommand = false;
	usize currentMulticommandSize = 0;

	auto freeSetFieldCommand(const SetFieldCommand& command) -> void;
	auto allocate(usize size) -> usize;
	auto freeTop() -> void;

	auto topHeader() -> u64&;

	std::vector<u8> data;
};