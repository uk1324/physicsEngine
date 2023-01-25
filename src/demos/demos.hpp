#pragma once

#include <gfx/gfx.hpp>
#include <game/renderer.hpp>
#include <demos/fourierTransformDemo.hpp>
#include <demos/eulerianFluid.hpp>
#include <demos/pixelPhysics.hpp>
#include <demos/triangulationDemo.hpp>
#include <demos/waterDemo.hpp>
#include <demos/testDemo.hpp>

struct Demos {
	Demos(Gfx& gfx);
	auto update(Gfx& gfx, Renderer& renderer) -> void;

	FourierTransformDemo fourierTransformDemo;
	EulerianFluidDemo eulerianFluid;
	PixelPhysics pixelPhysics;
	TriangulationDemo triangulationDemo;
	WaterDemo waterDemo;
	TestDemo testDemo;
};