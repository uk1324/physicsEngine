#pragma once

#include <game/body.hpp>
#include <game/collision/collision.hpp>
#include <unordered_map>

struct BodyPair {
	BodyPair(Body* b1, Body* b2) {
		if (b1 < b2) {
			body1 = b1;
			body2 = b2;
		}
		else {
			body1 = b2;
			body2 = b1;
		}
	}

	auto operator==(const BodyPair& other) const -> bool {
		return body1 == other.body1 && body2 == other.body2;
	}

	Body* body1;
	Body* body2;
};

struct BodyPairHasher {
	auto operator()(const BodyPair& x) const -> size_t {
		return reinterpret_cast<usize>(x.body1) * reinterpret_cast<usize>(x.body2);
	}
};

using CollisionMap = std::unordered_map<BodyPair, Collision, BodyPairHasher>;