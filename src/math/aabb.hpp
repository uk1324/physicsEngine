#pragma once

#include <utils/span.hpp>
#include <math/vec2.hpp>

struct Aabb {
	Aabb(Vec2 min, Vec2 max);

	static auto fromPoints(Span<const Vec2> points) -> Aabb;

	auto size() const -> Vec2;
	auto contains(Vec2 p) const -> bool;
	auto combined(const Aabb& aabb) const -> Aabb;
	auto area() const -> float;
	auto collides(const Aabb& other) const -> bool;
	auto rayHits(Vec2 start, Vec2 end) const -> bool;

	Vec2 min;
	Vec2 max;
};