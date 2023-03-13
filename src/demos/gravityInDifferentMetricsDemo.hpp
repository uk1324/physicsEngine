#pragma once

#include <engine/renderer.hpp>

struct GravityInDifferentMetricsDemo {
	GravityInDifferentMetricsDemo();
	auto update() -> void;
	auto metric(Vec2 v) -> float;
	static auto nextRandomColor() -> Vec3;

	Camera camera;

	struct Body {
		Vec2 pos;
		float radius;
		float mass;
		Vec2 vel{ 0.0f };
		Vec3 color;
		bool isStatic;
		std::vector<Vec2> trial;
	};
	std::vector<Body> bodies;

	float selectedRadius = 0.1f;
	float selectedMass = 1.0f;
	bool selectedIsStatic = false;
	Vec3 selectedColor;
	float gravitationalConstant = 1.0f;
	// When the cool value is -1 or other negative numbers then the orbit traces out the circle of that norm. 
	// Girves the unit circle if the it is symmetric around both axis. For example doesn't work for triangle. Also doesn't work for dicrete where the unity circle is the whole plane.
	// It seems to work better when the initial velocity is small.
	// Making mass difference bigger makes the movement faster, but it also works if the masses are equal.
	// The initial velocity can't be zero.
	// It also looks cool when the initial velocity is high.
	// The graivty equation assuming equal masses simplifies to
	// force = -distance * metric(distance)
	// Which is just a nonlinear spring.
	// Equations like x'' = -x|x| or  x'' = -x^3 can be solved in wolfram alpha, but they aren't algebraic.
	// It might be possible to solve the system of the spring in 2d, because you can rotate it to align with the axis (is this possible if the current velocity moves in a different direction?), but it would also require scaling, because the metric isn't symmetric.
	bool cool = false;
	float coolValue = 2.0f;
	
	int speed = 1;

	enum class Metric {
		LP_NORM, // if p < 1 this isn't actually a metric, because it doesn't satisfy the triangle inequality. The metric that uses the Lp norm is also called Minkowski distance.
		// When the p is negative it can create cool signed distance fields, but it doesn't really work well for planets.
		CHEBYSHEV,
		DISCRETE,
		P_ADIC,
		TRIANGLE, // Taken from https://www.shadertoy.com/view/NsScDV
		SIN,
		// Could also implement https://en.wikipedia.org/wiki/F-space.
	};

	Metric selectedMetric = Metric::LP_NORM;
	float pNormP = 2.0f;
	int pAdicP = 2;
	float sinPow = 2.0f;

	std::optional<Vec2> selectedPos;
	bool isPaused = false;
};