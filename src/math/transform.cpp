#include <math/transform.hpp>

Rotation::Rotation(float cos, float sin)
	: sin{ sin }
	, cos{ cos } {}

Rotation::Rotation(float angle)
	: sin{ ::sin(angle) }
	, cos{ ::cos(angle) } {}

Transform::Transform(Vec2 pos, Rotation rot)
	: pos{ pos }
	, rot{ rot } {}

Transform::Transform(Vec2 pos, float angle)
	: pos{ pos }
	, rot{ angle } {}

auto Rotation::inversed() const -> Rotation {
	return Rotation{ cos, -sin };
}

auto operator*(const Vec2& v, const Rotation& rot) -> Vec2 {
	return v.x * Vec2{ rot.cos, rot.sin } + v.y * Vec2{ -rot.sin, rot.cos };
}

auto operator*(const Vec2& v, const Transform& transform) -> Vec2 {
	return v * transform.rot + transform.pos;
}
