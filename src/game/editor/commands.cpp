#include <game/editor/commands.hpp>
#include <utils/overloaded.hpp>
#include <utils/io.hpp>

#define DEBUG_LOG_COMMANDS

static auto alignUpTo(usize v, usize alignment) -> usize {
    const auto unalignment = (v % alignment);
    return unalignment == 0
        ? v
        : v + (alignment - unalignment);
}

using Header = u64;

auto Commands::beginMulticommand() -> void {
#ifdef DEBUG_LOG_COMMANDS
    put("begin multicommand\n");
#endif
    ASSERT(!recordingMulticommand);
    recordingMulticommand = true;
    currentMulticommandSize = 0;
}

auto Commands::endMulticommand() -> void {
#ifdef DEBUG_LOG_COMMANDS
    put("end multicommand\n");
#endif

    ASSERT(recordingMulticommand);
    // Not asserting that the multicommand isn't empty because it allows writing easier code when adding commands in a loop over a range which can be empty.
    recordingMulticommand = false;
    if (currentMulticommandSize != 0) {
        commandSizes.push_back(currentMulticommandSize);
        commandsSizesTop++;
    }
}

auto Commands::addCommand(Command&& command) noexcept -> void {
#define LOG(type) [](const type&) -> void { put("added command %s", #type); },
    
#ifdef DEBUG_LOG_COMMANDS
    std::visit(overloaded{
        LOG(SetFieldCommand)
        LOG(SelectCommand)
        LOG(CreateEntityCommand)
        LOG(DeleteEntityCommand)
    }, command);
#undef LOG

    if (const auto& select = std::get_if<SelectCommand>(&command)) {
        put(" new{ ");
        for (const auto& newEntity : select->newSelectedEntites) {
            put("%d ", newEntity.index);
        }
        put("}");

        put(" old{ ");
        for (const auto& oldEntity : select->oldSelectedEntites) {
            put("%d ", oldEntity.index);
        }
        put("}");
    }

#endif 
    put("\n");


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
    if (recordingMulticommand) {
        currentMulticommandSize++;
    } else {
        commandsSizesTop++;
        commandSizes.push_back(1);
    }
    commandStack.push_back(std::move(command));
}

auto Commands::addSetFieldCommand(const Entity& entity, usize pointerOffset, const void* oldValue, const void* newValue, u8 size) -> void {
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
