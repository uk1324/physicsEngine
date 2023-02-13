#include <game/distanceJoint.hpp>
#include <game/ent.hpp>
#include <math/utils.hpp>

auto DistanceJoint::preStep(float invDeltaTime) -> void {
	if (!ent.body.isAlive(bodyA) || !ent.body.isAlive(bodyB)) {
		ent.distanceJoint.destroy(*this);
	}
	bias = invDeltaTime;
}

// Don't know what a correct pendulum should look like. Car keys that were left in the ignition were oscillating back and forth for around 3 minutes and if no one interrupted them, they would have continuted for a bit longer. The car keys were connected by a circle so the friction is should probably different from this kind of joint.

#include <utils/dbg.hpp>
auto DistanceJoint::applyImpluse() -> void {
	auto a = ent.body.get(bodyA);
	auto b = ent.body.get(bodyB);
	if (!a.has_value() || !b.has_value())
		return;

	const auto posOnA = a->transform.pos + anchorOnA * a->transform.rot;
	const auto posOnB = b->transform.pos + anchorOnB * b->transform.rot;
	const auto bToA = posOnA - posOnB;
	const auto distanceAb = bToA.length();
	const auto distanceToFix = distanceAb - requiredDistance;
	

	//Vec2 r1{ 0.0f }, r2{ 0.0f };
	
	const auto n = bToA.normalized();
	Vec2 r1 = posOnA - a->transform.pos, r2 = posOnB - b->transform.pos;
	float rn1 = dot(r1, n);
	float rn2 = dot(r2, n);
	float kNormal = a->invMass + b->invMass;
	kNormal += a->invRotationalInertia * (dot(r1, r1) - rn1 * rn1) + b->invRotationalInertia * (dot(r2, r2) - rn2 * rn2);
	//kNormal += a->invRotationalInertia * pow(cross(n, r1), 2.0f) + b->invRotationalInertia * pow(cross(n, r2), 2.0f);

	//auto relativeVel = b->vel + posOnB.rotBy90deg() * b->angularVel - (a->vel + posOnA.rotBy90deg() * a->angularVel);
	auto relativeVel = (b->vel + cross(b->angularVel, r2)) - (a->vel + cross(a->angularVel, r1));
	float vn = dot(relativeVel, n);

	// Try to zero the velocity by pushing in the direction opposite to the error.
	// If too close then push away and if too far away push together.
	if (distanceToFix < 0.0f) {
		vn = std::max(vn, 0.0f);
	} else {
		vn = std::min(vn, 0.0f);
	}
	/*float dPn = (distanceToFix * bias - vn) / kNormal / 4.0f;*/
	float dPn = (distanceToFix - vn) / kNormal;
	/*float dPn = (distanceToFix - vn) / kNormal;*/
	//float dPn = (distanceToFix * bias - vn) / kNormal;

	a->vel -= dPn * n * a->invMass;
	b->vel += dPn * n * b->invMass;
	a->angularVel -= a->invRotationalInertia * cross(r1, dPn * n);
	b->angularVel += b->invRotationalInertia * cross(r2, dPn * n);

	relativeVel = b->vel - a->vel;
	Vec2 tangent = n.rotBy90deg();




	//float vt = dot(relativeVel, tangent);

	//float coefficientOfFriction = 0.2f;
	//float aaa = dPn * pow(coefficientOfFriction, bias);
	//float maxPt = vt < 0.0f ? -vt : vt;
	//float out;
	//aaa = std::clamp(aaa, -maxPt, maxPt);
	//float minPt = (maxPt < 0.0f) ? maxPt : -maxPt;

	//Vec2 Pt = aaa * tangent * -sign(vt);
	//Debug::drawRay(b.pos, Pt);

	//a.vel -= a.invMass * Pt;
	//a.angularVel -= a.invRotationalInertia * det(r1, Pt);

	//b.vel += b.invMass * Pt;
	//b.angularVel += b.invRotationalInertia * det(r2, Pt);




	float vt = dot(relativeVel, tangent);
	//float dPt = (-vt * bias * 0.016f);
	float dPt = (-vt);

	float coefficientOfFriction = 0.2f;
	float maxPt = coefficientOfFriction * dPn;
	/*dPt = std::clamp(dPt, -maxPt, maxPt);*/
	float minPt = (maxPt < 0.0f) ? maxPt : -maxPt;
	dPt = std::clamp(dPt, minPt, -minPt);

	Vec2 Pt = dPt * tangent;

	a->vel -= a->invMass * Pt;
	a->angularVel -= a->invRotationalInertia * det(r1, Pt);

	b->vel += b->invMass * Pt;
	b->angularVel += b->invRotationalInertia * det(r2, Pt);
}

auto DistanceJoint::getEndpoints() const -> std::array<Vec2, 2> {
	const auto a = ent.body.get(bodyA);
	const auto b = ent.body.get(bodyB);
	if (!a.has_value() || !b.has_value()) {
		//ASSERT_NOT_REACHED();
		return { Vec2{ 0.0f }, Vec2{ 0.0f } };
	}

	return { a->transform.pos + anchorOnA * a->transform.rot, b->transform.pos + anchorOnB * b->transform.rot };
}
