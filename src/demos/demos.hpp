#pragma once

#include <demos/fourierTransformDemo.hpp>
#include <demos/eulerianFluid.hpp>
#include <demos/pixelPhysics.hpp>
#include <demos/triangulationDemo.hpp>
#include <demos/waterDemo.hpp>
#include <demos/testDemo.hpp>

struct Demos {
	auto update() -> void;

	FourierTransformDemo fourierTransformDemo;
	EulerianFluidDemo eulerianFluid;
	PixelPhysics pixelPhysics;
	TriangulationDemo triangulationDemo;
	WaterDemo waterDemo;
	TestDemo testDemo;
};