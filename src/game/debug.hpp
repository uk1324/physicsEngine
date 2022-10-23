#pragma once

#include <math/vec2.hpp>
#include <math/vec3.hpp>
#include <utils/span.hpp>
#include <vector>

namespace Debug {
	auto update() -> void;

	auto drawLine(Vec2 start, Vec2 end, const Vec3& color = Vec3{ 1.0f, 1.0f, 1.0f }) -> void;
	auto drawCircle(Vec2 pos, float radius = 0.01f, const Vec3& color = Vec3{ 1.0f, 1.0f, 1.0f }) -> void;
	auto drawPoint(Vec2 pos, const Vec3& color = Vec3{ 1.0f, 1.0f, 1.0f }) -> void;
	auto drawLines(Span<const Vec2> lines, const Vec3& color = Vec3{ 1.0f, 1.0f, 1.0f }) -> void;

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

	extern std::vector<Line> lines;
	extern std::vector<Circle> circles;
}