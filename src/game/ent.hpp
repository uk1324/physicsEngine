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