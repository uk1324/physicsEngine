#pragma once

#include <game/collision/collision.hpp>
#include <unordered_map>

struct BodyPair {
	BodyPair(Body* b1, Body* b2) {
		ASSERT(b1 != b2);
		if (b1 < b2) {
			a = b1;
			b = b2;
		}
		else {
			a = b2;
			b = b1;
		}
	}

	// TODO: Convert this to =default
	auto operator==(const BodyPair& other) const -> bool {
		return a == other.a && b == other.b;
	}

	Body* a;
	Body* b;
};

struct BodyPairHasher {
	auto operator()(const BodyPair& x) const -> size_t {
		return reinterpret_cast<usize>(x.a) * reinterpret_cast<usize>(x.b);
	}
};

// unordered_map shouldn't be used here because it makes the collisions be resolved in a different order depending on the allocation location and the insertion order. Using a std::map and making a custom comparasion function would work. There must a consistent way to order the entites. This could be done be either using handles instead of pointers or storing an index inside the Body.
using CollisionMap = std::unordered_map<BodyPair, Collision, BodyPairHasher>;