#include <game/camera.hpp>
#include <math/utils.hpp>

Camera::Camera(Vec2 pos, float zoom)
	: pos{ pos }
	, zoom{ zoom } {}

auto Camera::interpolateTo(Vec2 desiredPos, float speed) -> void {
	pos = lerp(pos, desiredPos, speed);
}

auto Camera::cameraTransform() const -> Mat3x2 {
	return Mat3x2::translate(-pos * Vec2{ 1.0f, aspectRatio }) * Mat3x2::scale(Vec2{ zoom });
}

auto Camera::screenSpaceToCameraSpace(Vec2 screenSpacePos) -> Vec2 {
	return (screenSpacePos * Vec2{ 1.0f, 1.0f / aspectRatio } / zoom) + pos;
}

// Assumes aspect ratio is width / height.
auto Camera::heightIfWidthIs(float width) -> float {
	return width / aspectRatio;
}