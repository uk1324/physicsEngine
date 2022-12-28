#pragma once

#include <math/mat3x2.hpp>

struct Camera {
	Camera(Vec2 pos = Vec2{ 0.0f }, float zoom = 1.0f);

	auto interpolateTo(Vec2 desiredPos, float speed) -> void;
	auto cameraTransform() const-> Mat3x2;
	auto screenSpaceToCameraSpace(Vec2 screenSpacePos)->Vec2;
	auto heightIfWidthIs(float width) -> float;

	Vec2 pos;
	float zoom;
	float aspectRatio = 1.0f;
};