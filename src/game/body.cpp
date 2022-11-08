#include <game/body.hpp>
#include <math/vec2.hpp>
#include <numeric>


Body::Body(Vec2 pos, const Collider& collider, bool isStatic) 
	: pos{ pos }
	, collider{ collider }
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
		static constexpr float DENSITY = 200.0f;
		const auto info = massInfo(collider, DENSITY);
		mass = info.mass;
		invMass = 1.0f / info.mass;
		rotationalInertia = info.rotationalInertia;
		invRotationalInertia = 1.0f / info.rotationalInertia;
	}

	coefficientOfFriction = 0.2f;

	torque = 0.0f;
	force = Vec2{ 0.0f };
}
