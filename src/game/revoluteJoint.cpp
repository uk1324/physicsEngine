#include <game/revoluteJoint.hpp>
#include <game/ent.hpp>

auto RevoluteJoint::preStep(float invDeltaTime) -> void {
	bias = invDeltaTime;
	auto a = ent.body.get(bodyA);
	auto b = ent.body.get(bodyB);
	if (!a.has_value() || !b.has_value()) {
		return;
	}
	auto rA = localAnchorA * a->transform.rot;
	auto rB = localAnchorB * b->transform.rot;
	m[0][0] = rA.y * rA.y * a->invRotationalInertia + rB.y * rB.y * b->invRotationalInertia + a->invMass + b->invMass;
	m[1][0] = m[0][1] = -rA.y * rA.x * a->invRotationalInertia - rB.y * rB.x * b->invRotationalInertia;
	m[1][1] = rA.x * rA.x * a->invRotationalInertia + rB.x * rB.x * b->invRotationalInertia + a->invMass + b->invMass;
	m = m.inversed();
}

#include <engine/debug.hpp>

auto RevoluteJoint::applyImpluse() -> void {
	auto a = ent.body.get(bodyA);
	auto b = ent.body.get(bodyB);
	if (!a.has_value() || !b.has_value()) {
		return;
	}

	// TODO: make function from object space to world space? Does it obscure anything?
	/*const auto anchorA = a->transform.pos + anchorOnA * a->transform.rot;
	const auto anchorB = b->transform.pos + anchorOnB * a->transform.rot;*/
	auto rA = localAnchorA * a->transform.rot;
	auto rB = localAnchorB * b->transform.rot;
	/*m[0][0] = rA.y * rA.y * a->invRotationalInertia + rB.y * rB.y * b->invRotationalInertia + a->invMass + b->invMass;
	m[1][0] = m[0][1] = -rA.y * rA.x * a->invRotationalInertia - rB.y * rB.x * b->invRotationalInertia;
	m[1][1] = rA.x * rA.x * a->invRotationalInertia + rB.x * rB.x * b->invRotationalInertia + a->invMass + b->invMass;*/
	auto relativeVel = (b->vel + cross(b->angularVel, rB)) - (a->vel + cross(a->angularVel, rA));
	const auto error = (b->transform.pos + rB) - (a->transform.pos + rA);
	const auto impulse = -(relativeVel + error * bias * 0.2f) * m;

	a->vel -= a->invMass * impulse;
	a->angularVel -= a->invRotationalInertia * cross(rA, impulse);
	b->vel += b->invMass * impulse;
	b->angularVel += b->invRotationalInertia * cross(rB, impulse);
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
