#include <game/editor/commands.hpp>
#include <utils/overloaded.hpp>

static auto alignUpTo(usize v, usize alignment) -> usize {
    const auto unalignment = (v % alignment);
    return unalignment == 0
        ? v
        : v + (alignment - unalignment);
}

using Header = u64;

auto Commands::beginMulticommand() -> void {
    ASSERT(!multicommandStarted);
    multicommandStarted = true;
    currentMulticommandSize = 0;
}

auto Commands::endMulticommand() -> void {
    ASSERT(multicommandStarted);
    multicommandStarted = false;
    commandSizes.push_back(currentMulticommandSize);
}

auto Commands::addCommand(Command&& command) noexcept -> void {
    ASSERT(commandsSizesTop <= commandSizes.size());
    if (commandsSizesTop != commandSizes.size()) {
        const auto toFree = commandSizes.size() - commandsSizesTop;
        for (usize i = 0; i < toFree; i++) {
            for (usize j = 0; j < commandSizes.back(); j++) {
                auto& commandToDelete = commandStack[commandStack.size() - 1];
                if (const auto& setField = std::get_if<SetFieldCommand>(&commandToDelete); setField != nullptr) {
                    freeSetFieldCommand(*setField);
                }
                commandStack.pop_back();
            }
            commandSizes.pop_back();
        }
        commandsSizesTop = commandSizes.size();
    }
    if (multicommandStarted) {
        currentMulticommandSize++;
    } else {
        commandsSizesTop++;
        commandSizes.push_back(1);
    }
    commandStack.push_back(std::move(command));
}

auto Commands::addSetFieldCommand(const Entity& entity, usize pointerOffset, void* oldValue, void* newValue, u8 size) -> void {
    const auto oldPtr = allocate(static_cast<usize>(size) * 2);
    const auto newPtr = oldPtr + size;
    memcpy(getPtr(oldPtr), oldValue, size);
    memcpy(getPtr(newPtr), newValue, size);
    addCommand(SetFieldCommand{
        .entity = entity,
        .size = size,
        .pointerOffset = pointerOffset,
        .oldDataPtr = oldPtr,
        .newDataPtr = newPtr,
    });
}

auto Commands::addSelectCommand(Span<const Entity> oldSelectedEntites, Span<const Entity> newSelectedEntites) -> void {
    addCommand(SelectCommand{
        .oldSelectedEntites = std::vector<Entity>{ oldSelectedEntites.begin(), oldSelectedEntites.end() },
        .newSelectedEntites = std::vector<Entity>{ newSelectedEntites.begin(), newSelectedEntites.end() },
    });
}

auto Commands::getPtr(usize ptr) -> u8* {
    return data.data() + ptr;
}

auto Commands::freeSetFieldCommand(const SetFieldCommand&) -> void {
    freeTop();
}

auto Commands::allocate(usize size) -> usize {
    const auto allocationSize = alignUpTo(size, alignof(Header)) + sizeof(Header);
    const auto ptr = data.size();
    data.resize(data.size() + allocationSize);
    topHeader() = allocationSize;
    return ptr;
}

auto Commands::freeTop() -> void {
    if (data.size() >= topHeader()) {
        data.resize(data.size() - topHeader());
    } else {
        ASSERT_NOT_REACHED();
    }
}

auto Commands::topHeader() -> u64& {
    return *reinterpret_cast<u64*>(data.data() + data.size() - sizeof(Header));
}
