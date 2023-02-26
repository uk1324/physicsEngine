#include <game/revoluteJoint.hpp>
#include <game/ent.hpp>

auto RevoluteJoint::preStep(float invDeltaTime) -> void {
	bias = invDeltaTime;
	auto a = ent.body.get(bodyA);
	auto b = ent.body.get(bodyB);
	if (!a.has_value() || !b.has_value()) {
		ent.revoluteJoint.destroy(*this);
		return;
	}
	const auto rA = localAnchorA * a->transform.rot;
	const auto rB = localAnchorB * b->transform.rot;
	if (a->isStatic() && b->isStatic()) {
		// Without this it creates a zero matrix. 
		m = Mat2::identity;
	} else {
		m[0][0] = rA.y * rA.y * a->invRotationalInertia + rB.y * rB.y * b->invRotationalInertia + a->invMass + b->invMass;
		m[1][0] = m[0][1] = -rA.y * rA.x * a->invRotationalInertia - rB.y * rB.x * b->invRotationalInertia;
		m[1][1] = rA.x * rA.x * a->invRotationalInertia + rB.x * rB.x * b->invRotationalInertia + a->invMass + b->invMass;
		m = m.inversed();
	}
	
}

auto RevoluteJoint::applyImpluse() -> void {
	auto a = ent.body.get(bodyA);
	auto b = ent.body.get(bodyB);
	if (!a.has_value() || !b.has_value()) {
		return;
	}

	const auto rA = localAnchorA * a->transform.rot;
	const auto rB = localAnchorB * b->transform.rot;
	const auto relativeVel = (b->vel + cross(b->angularVel, rB)) - (a->vel + cross(a->angularVel, rA));
	const auto error = (b->transform.pos + rB) - (a->transform.pos + rA);
	const auto impulse = -(relativeVel + error * bias * 0.2f) * m;

	a->vel -= a->invMass * impulse;
	a->angularVel -= a->invRotationalInertia * cross(rA, impulse);
	b->vel += b->invMass * impulse;
	b->angularVel += b->invRotationalInertia * cross(rB, impulse);

	if (motorSpeedInRadiansPerSecond != 0.0f) {
		const auto lambda = 1.0f / (a->invRotationalInertia + b->invRotationalInertia);
		auto motorImpulse = (a->angularVel - b->angularVel + motorSpeedInRadiansPerSecond) * lambda;
		motorImpulse = std::clamp(motorImpulse, -1000.0f, 1000.0f);
		a->angularVel -= motorImpulse * a->invMass;
		b->angularVel += motorImpulse * b->invMass;
	}
}

auto RevoluteJoint::anchorsWorldSpace() const -> std::array<Vec2, 2> {
	const auto a = ent.body.get(bodyA);
	const auto b = ent.body.get(bodyB);
	if (!a.has_value() || !b.has_value()) {
		return { Vec2{ 0.0f }, Vec2{ 0.0f } };
	}
	return {
		a->transform.pos + localAnchorA * a->transform.rot,
		b->transform.pos + localAnchorB * b->transform.rot
	};
}
