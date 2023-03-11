#pragma once

#include <demos/fourierTransformDemo.hpp>
#include <demos/eulerianFluid.hpp>
#include <demos/pixelPhysics.hpp>
#include <demos/triangulationDemo.hpp>
#include <demos/waterDemo.hpp>
#include <demos/testDemo.hpp>
#include <demos/marchingSquaresDemo.hpp>
#include <demos/integrationDemo.hpp>
#include <demos/polygonHull.hpp>
#include <demos/imageToSdf.hpp>
#include <demos/gravityInDifferentMetricsDemo.hpp>

struct Demos {
	auto update() -> void;

	//FourierTransformDemo fourierTransformDemo;
	//EulerianFluidDemo eulerianFluid;
	//PixelPhysics pixelPhysics;
	//TriangulationDemo triangulationDemo;
	//WaterDemo waterDemo;
	//TestDemo testDemo;
	//MarchingSquaresDemo marchingSquaresDemo;
	//IntegrationDemo integrationDemo;
	//PolygonHullDemo polygonHullDemo;
	ImageToSdfDemo imageToSdfDemo;
	GravityInDifferentMetricsDemo gravityInDifferentMetricsDemo;
};