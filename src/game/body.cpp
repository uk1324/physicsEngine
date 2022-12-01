#include <game/entitesData.hpp>
#include <game/collision/collision.hpp>
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

auto Body::updateInvMassAndInertia() -> void {
	if (mass == std::numeric_limits<float>::infinity()) {
		invMass = 0.0f;
		invRotationalInertia = 0.0f;
		rotationalInertia = std::numeric_limits<float>::infinity();
	} else {
		invMass = 1.0f / mass;
		invRotationalInertia  = 1.0f / rotationalInertia;
	}
}

Body::Body() : Body{ Vec2{ 0.0f }, CircleCollider{ 0.5f }, false } {}
