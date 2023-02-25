#pragma once

#include <game/entityArray.hpp>
#include <game/body.hpp>
#include <game/distanceJoint.hpp>
#include <game/revoluteJoint.hpp>
#include <game/collisionSystem.hpp>
#include <game/trail.hpp>
#include <math/vec3.hpp>

#include <unordered_set>

using IgnoredCollisions = std::unordered_set<BodyPair, BodyPairHasher>;

struct Entites {
	EntityArray<Body> body;
	EntityArray<DistanceJoint> distanceJoint;
	EntityArray<RevoluteJoint> revoluteJoint;
	EntityArray<Trail> trail;

	IgnoredCollisions collisionsToIgnore;
	std::vector<std::pair<DistanceJointId, BodyPair>> revoluteJointsWithIgnoredCollisions;

	auto update() -> void;
	auto reset() -> void;
};

extern Entites ent;

#include <game/entMacro.hpp>

#define ID(Name, name) Name##Id
using Entity = std::variant<
	ENTITY_TYPE_LIST_COMMA_SEPARATED(ID)
>;
#undef ID

#include <game/entMacroUndef.hpp>

auto entityIsAlive(const Entity& entity) -> bool;
auto entityDestroy(const Entity& entity) -> void;
auto entityIdIndex(const Entity& entity) -> int;