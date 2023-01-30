#pragma once

#include <game/entityArray.hpp>
#include <game/body.hpp>
#include <game/distanceJoint.hpp>

struct Entites {
	EntityArray<Body> body;
	EntityArray<DistanceJoint> distanceJoint;

	auto update() -> void;
	auto reset() -> void;
};

extern Entites ent;