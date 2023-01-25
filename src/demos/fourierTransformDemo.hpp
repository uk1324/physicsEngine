#pragma once

#include <engine/renderer.hpp>
#include <complex>

struct FourierTransformDemo {
	FourierTransformDemo();

	auto update() -> void;

	Camera camera;

	bool displayMagnitudeIfFalsePhaseIfTrue = false;
	bool transformed = false;
	int brushSize = 4;
	u8 brushColor = 255;
	static constexpr auto IMAGE_SIZE = 1 << 8;
	DynamicTexture texture;
	std::complex<double> fourierTransform[IMAGE_SIZE][IMAGE_SIZE];
};