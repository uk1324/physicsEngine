#pragma once

#include <math/vec2.hpp>
#include <math/vec3.hpp>
#include <vector>

namespace Debug {
	auto update() -> void;

	auto drawLine(Vec2 start, Vec2 end, const Vec3& color = Vec3{ 1.0f, 1.0f, 1.0f }) -> void;

	struct Line {
		Vec2 start;
		Vec2 end;
		Vec3 color;
	};

	extern std::vector<Line> lines;
}