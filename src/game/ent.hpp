#pragma once

#include <game/entityArray.hpp>
#include <game/body.hpp>
#include <game/distanceJoint.hpp>
#include <game/revoluteJoint.hpp>
#include <game/springJoint.hpp>
#include <game/collisionSystem.hpp>
#include <game/trail.hpp>
#include <math/vec3.hpp>

#include <unordered_set>

using IgnoredCollisions = std::unordered_set<BodyPair, BodyPairHasher>;

#include <game/entMacro.hpp>

struct Entites {
#define ARRAY(Name, name) EntityArray<Name> name;
	ENTITY_TYPE_LIST(ARRAY,)
#undef ARRAY

	IgnoredCollisions collisionsToIgnore;
	std::vector<std::pair<DistanceJointId, BodyPair>> revoluteJointsWithIgnoredCollisions;

	auto update() -> void;
	auto reset() -> void;
};

extern Entites ent;

#define ID(Name, name) Name##Id
using Entity = std::variant<
	ENTITY_TYPE_LIST_COMMA_SEPARATED(ID)
>;
#undef ID

#include <game/entMacroUndef.hpp>

auto entityIsAlive(const Entity& entity) -> bool;
auto entityDestroy(const Entity& entity) -> void;
auto entityIdIndex(const Entity& entity) -> int;