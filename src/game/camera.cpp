#include <game/camera.hpp>
#include <math/utils.hpp>
#include <math/aabb.hpp>

Camera::Camera(Vec2 pos, float zoom)
	: pos{ pos }
	, zoom{ zoom } {}

auto Camera::posInGrid(Vec2 pos, Vec2 gridCenter, float gridSize, Vec2T<i64> gridCellSize) -> Vec2T<i64> {
	const Vec2 size{ gridSize * gridCellSize.xOverY(), gridSize };
	const auto textureBox = Aabb::fromPosSize(gridCenter, size);
	auto gridPos = pos - Vec2{ textureBox.min.x, textureBox.max.y };
	gridPos /= size;
	gridPos.y = -gridPos.y;
	return Vec2T<i64>{ (gridPos * Vec2{ gridCellSize }).applied(floor) };
}

auto Camera::interpolateTo(Vec2 desiredPos, float speed) -> void {
	pos = lerp(pos, desiredPos, speed);
}

auto Camera::cameraTransform() const -> Mat3x2 {
	return Mat3x2::translate(-pos * Vec2{ 1.0f, aspectRatio }) * Mat3x2::scale(Vec2{ zoom });
}

auto Camera::height() const -> float {
	return 2.0f / aspectRatio / zoom;
}

auto Camera::width() const -> float {
	return 2.0f / zoom;
}

auto Camera::screenSpaceToCameraSpace(Vec2 screenSpacePos) const -> Vec2 {
	return (screenSpacePos * Vec2{ 1.0f, 1.0f / aspectRatio } / zoom) + pos;
}

auto Camera::heightIfWidthIs(float width) const -> float {
	return width / aspectRatio;
}