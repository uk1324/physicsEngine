#include <game/body.hpp>
#include <math/vec2.hpp>
#include <numeric>

static constexpr float DENSITY = 200.0f;

static auto rectangleInertia(Vec2 size, float mass) -> float {
	return mass * pow(size.x, 2.0f) * pow(size.y, 2.0f) / 12.0f;
}

Body::Body(Vec2 pos, Vec2 size, bool isStatic) 
	: pos{ pos }
	, size{ size }
	, orientation{ 0.0f }
	, angularVel{ 0.0f }
	, vel{ 0.0f } {

	if (isStatic) {
		mass = std::numeric_limits<float>::infinity();
		invMass = 0.0f;
		rotationalInertia = std::numeric_limits<float>::infinity();
		invRotationalInertia = 0.0f;
	}
	else {
		mass = size.x * size.y * DENSITY;
		invMass = 1.0f / mass;
		rotationalInertia = rectangleInertia(size, mass);
		invRotationalInertia = 1.0f / rotationalInertia;
	}

	coefficientOfFriction = 0.2f;

	torque = 0.0f;
	force = Vec2{ 0.0f };
}
