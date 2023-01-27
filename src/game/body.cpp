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

Body::Body(const BodyOldEditor& body)
	: invMass{ 1.0f / body.mass }
	, invRotationalInertia{ 1.0f / body.rotationalInertia }
	, torque{ 0.0f }
	, force{ 0.0f }
	, collider{ body.collider }
	, mass{ body.mass }
	, rotationalInertia{ body.rotationalInertia } 
	, transform{ body.pos, body.orientation }
	, vel{ body.vel }
	, angularVel{ body.angularVel }
	, coefficientOfFriction{ 0.2f }
{}

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

Body::Body() : Body{ Vec2{ 0.0f }, CircleColliderEditor{ 0.5f }, false } {}
