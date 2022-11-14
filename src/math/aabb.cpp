#include <math/aabb.hpp>

Aabb::Aabb(Vec2 min, Vec2 max)
	: min{ min }
	, max{ max } {}

auto Aabb::fromPoints(Span<const Vec2> points) -> Aabb {
	Vec2 min{ std::numeric_limits<float>::infinity() }, max{ -std::numeric_limits<float>::infinity() };

	for (const auto& point : points) {
		min = min.min(point);
		max = max.max(point);
	}

	return Aabb{ min, max };
}

auto Aabb::size() const -> Vec2 {
	return max - min;
}

auto Aabb::contains(Vec2 p) const -> bool {
	return p.x >= min.x && p.x <= max.x
		&& p.y >= min.y && p.y <= max.y;
}

auto Aabb::combined(const Aabb& aabb) const -> Aabb {
	return Aabb{ min.min(aabb.min), max.max(aabb.max) };
}

auto Aabb::area() const -> float {
	const auto s = size();
	return s.x * s.y;
}

auto Aabb::collides(const Aabb& other) const -> bool {
	return min.x <= other.max.x && max.x >= other.min.x
		&& min.y <= other.max.y && max.y >= other.min.y;
}
