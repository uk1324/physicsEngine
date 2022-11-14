#pragma once

#include <math/aabb.hpp>
#include <math/vec2.hpp>
#include <math/vec3.hpp>
#include <utils/span.hpp>
#include <vector>

namespace Debug {
	auto update() -> void;

	static constexpr auto DEFAULT_COLOR = Vec3{ 1.0f, 1.0f, 1.0f };

	auto drawLine(Vec2 start, Vec2 end, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawRay(Vec2 start, Vec2 ray, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawCircle(Vec2 pos, float radius = 0.01f, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawEmptyCircle(Vec2 pos, float radius = 0.01f, float orientation = 0.0f, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawPoint(Vec2 pos, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawLines(Span<const Vec2> lines, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawAabb(const Aabb& aabb, const Vec3& color = DEFAULT_COLOR) -> void;

	struct Line {
		Vec2 start;
		Vec2 end;
		Vec3 color;
	};

	struct Circle {
		Vec2 pos;
		float radius;
		Vec3 color;
	};

	struct OrientedCircle {
		Circle circle;
		float orientation;
	};

	extern std::vector<Line> lines;
	extern std::vector<Circle> circles;
	extern std::vector<OrientedCircle> emptyCircles;
}