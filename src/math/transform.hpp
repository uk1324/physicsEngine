#pragma once

#include <math/vec2.hpp>

struct Rotation {
	Rotation(float cos, float sin);
	Rotation(float angle);
	auto inversed() const -> Rotation;
	float cos, sin;
};

struct Transform {
	Transform(Vec2 pos, Rotation rot);
	Transform(Vec2 pos, float angle);

	Vec2 pos;
	Rotation rot;
};

auto operator*(const Vec2& v, const Rotation& rot) -> Vec2;
auto operator*(const Vec2& v, const Transform& transform) -> Vec2;
