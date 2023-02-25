#include <game/ent.hpp>
#include <utils/overloaded.hpp>
#include <game/entMacro.hpp>

auto Entites::update() -> void {
#define UPDATE(Name, name) name.update();
	ENTITY_TYPE_LIST(UPDATE,)
#undef UPDATE

		// TODO: Make the joint do this in the desctructor. There might be some issues with destructors of classes like body trying to delete bodies. Because it would add to a list that is being iterated. This shouldn't be an issues in this case though because it would be removing from revoluteJointsWithIgnoredCollisions while iterating over distance joints to destroy.
	std::erase_if(revoluteJointsWithIgnoredCollisions, [this](const auto& it) {
		const auto& [joint, bodyPair] = it;
		if (!ent.distanceJoint.isAlive(joint)) {
			collisionsToIgnore.erase(bodyPair);
			return true;
		}
		return false;
	});
}

auto Entites::reset() -> void {
#define RESET(Name, name) name.reset();
	ENTITY_TYPE_LIST(RESET, )
#undef RESET
	collisionsToIgnore.clear();
	revoluteJointsWithIgnoredCollisions.clear();
}

auto entityIsAlive(const Entity& entity) -> bool {
#define IS_ALIVE(Name, name) [](const Name##Id& id) -> bool { return ent.name.isAlive(id); }
	return std::visit(overloaded{ ENTITY_TYPE_LIST_COMMA_SEPARATED(IS_ALIVE) }, entity);
#undef IS_ALIVE
}

auto entityDestroy(const Entity& entity) -> void {
#define DESTROY(Name, name) [](const Name##Id& id) -> void { ent.name.destroy(id); }
	std::visit(overloaded{ ENTITY_TYPE_LIST_COMMA_SEPARATED(DESTROY) }, entity);
#undef DESTROY
}

auto entityIdIndex(const Entity& entity) -> int {
#define INDEX(Name, name) [](const Name##Id& id) -> int { return id.index(); }
	return std::visit(overloaded{ ENTITY_TYPE_LIST_COMMA_SEPARATED(INDEX) }, entity);
#undef INDEX
}


Entites ent;
