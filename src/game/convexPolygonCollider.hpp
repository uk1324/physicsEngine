#pragma once

#include <math/vec2.hpp>
#include <math/utils.hpp>

#include <vector>

struct ConvexPolygon {
	// Could store the verts and normal into a vector of structs with edgeBegin and edgeNormal, but that would probably be confusing.

	std::vector<Vec2> verts;
	// The normal i belong to the face with endpoints verts[i] and verts[(i + 1) % size].
	std::vector<Vec2> normals;

	auto calculateNormals() -> void {
		normals.clear();
		if (verts.size() < 3) {
			ASSERT_NOT_REACHED();
			return;
		}

		for (usize i = 0; i < verts.size() - 1; i++) {
			normals.push_back((verts[i + 1] - verts[i]).rotBy90deg().normalized());
		}
		normals.push_back((verts[0] - verts.back()).rotBy90deg().normalized());
	}
};

inline auto makeRegularPolygon(i32 sides, float radius) -> ConvexPolygon {
	ConvexPolygon polygon;
	const auto angleStep = TAU<float> / sides;
	for (i32 i = 0; i < sides; i++) {
		polygon.verts.push_back(Vec2::oriented(angleStep / 2.0f + angleStep * i) * radius);
	}
	polygon.calculateNormals();
	return polygon;
};