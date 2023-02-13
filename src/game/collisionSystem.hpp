#pragma once

#include <game/collision.hpp>
#include <unordered_map>
#include <game/body.hpp>

struct BodyPair {
	BodyPair(BodyId bodyA, BodyId bodyB) {
		if (bodyA.index() < bodyB.index()) {
			a = bodyA;
			b = bodyB;
		}
		else {
			a = bodyB;
			b = bodyA;
		}
	}

	// TODO: Convert this to =default
	auto operator==(const BodyPair& other) const -> bool {
		return a == other.a && b == other.b;
	}

	BodyId a;
	BodyId b;
};

struct BodyPairHasher {
	auto operator()(const BodyPair& x) const -> size_t {
		return std::hash<EntityArray<Body>>()(x.a) * std::hash<EntityArray<Body>>()(x.b);
	}
};

namespace std {

template<>
struct hash<BodyPair> {
	using argument_type = BodyPair;
	using result_type = size_t;

	result_type operator ()(const argument_type& key) const {
		return BodyPairHasher{}(key);
	}
};

}

// unordered_map shouldn't be used here because it makes the collisions be resolved in a different order depending on the allocation location and the insertion order. Using a std::map and making a custom comparasion function would work. There must a consistent way to order the entites. This could be done be either using handles instead of pointers or storing an index inside the Body.
using CollisionMap = std::unordered_map<BodyPair, Collision, BodyPairHasher>;