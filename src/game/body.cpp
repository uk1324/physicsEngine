#include <game/body.hpp>
#include <game/collision.hpp>
#include <math/vec2.hpp>
#include <numeric>


Body::Body(Vec2 pos, const Collider& collider, bool isStatic) 
	: collider{ collider }
	, transform{ pos, 0.0f } {
	//this->pos = pos;
	//this->collider = collider;
	//orientation = 0.0f;
	//transform = /*Transform*/{  }
	angularVel = 0.0f;
	vel = Vec2{ 0.0f };

	if (isStatic) {
		mass = std::numeric_limits<float>::infinity();
		invMass = 0.0f;
		rotationalInertia = std::numeric_limits<float>::infinity();
		invRotationalInertia = 0.0f;
	}
	else {
		const auto info = massInfo(collider, DEFAULT_DENSITY);
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

auto Body::isStatic() const -> bool {
	return invMass == 0.0f;
}

auto Body::makeStatic() -> void {
	mass = std::numeric_limits<float>::infinity();
	updateInvMassAndInertia();
}

auto Body::updateMass(float density) -> void {
	const auto info = massInfo(collider, density);
	mass = info.mass;
	rotationalInertia = info.rotationalInertia;
	updateInvMassAndInertia();
}

Body::Body() : Body{ Vec2{ 0.0f }, CircleCollider{ 0.5f }, false } {}
