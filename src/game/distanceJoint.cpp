#include <game/distanceJoint.hpp>
#include <game/body.hpp>
#include <math/utils.hpp>

auto DistanceJoint::preStep(Body& a, Body& b, float invDeltaTime) -> void {
	bias = invDeltaTime;
}

#include <game/debug.hpp>

auto DistanceJoint::applyImpluse(Body& a, Body& b) -> void {
	const auto bToA = a.pos - b.pos;
	const auto distanceAb = bToA.length();
	const auto distanceToFix = distanceAb - requiredDistance;
	auto relativeVel = b.vel - a.vel;

	Vec2 r1{ 0.0f }, r2{ 0.0f };
	float rn1 = 0.0f;
	float rn2 = 0.0f;
	float kNormal = a.invMass + b.invMass;
	kNormal += a.invRotationalInertia * (dot(r1, r1) - rn1 * rn1) + b.invRotationalInertia * (dot(r2, r2) - rn2 * rn2);
	//c->invNormalEffectiveMass = 1.0f / kNormal;


	const auto n = bToA.normalized();
	float vn = dot(relativeVel, n);

	// Try to zero the velocity by pushing in the direction opposite to the error.
	// If too close then push away and if too far away push together.
	if (distanceToFix < 0.0f) {
		vn = std::max(vn, 0.0f);
	} else {
		vn = std::min(vn, 0.0f);
	}
	float dPn = (distanceToFix - vn) / kNormal;
	a.vel -= dPn * n * a.invMass;
	b.vel += dPn * n * b.invMass;

	relativeVel = b.vel - a.vel;
	Vec2 tangent = n.rotBy90deg();
	float rt1 = dot(r1, tangent);
	float rt2 = dot(r2, tangent);
	float kTangent = a.invMass + b.invMass;
	kTangent += a.invRotationalInertia * (dot(r1, r1) - rt1 * rt1) + b.invRotationalInertia * (dot(r2, r2) - rt2 * rt2);

	float vt = dot(relativeVel, tangent);
	// !!!! Not using kTangent here.
	float dPt = (-vt);

	float coefficientOfFriction = 0.2f;
	float maxPt = coefficientOfFriction * dPn;
	/*dPt = std::clamp(dPt, -maxPt, maxPt);*/
	float minPt = (maxPt < 0.0f) ? maxPt : -maxPt;
	dPt = std::clamp(dPt, minPt, -minPt);

	Vec2 Pt = dPt * tangent;

	Debug::drawRay(b.pos, Pt);
	a.vel -= a.invMass * Pt;
	/*a.angularVel -= a.invRotationalInertia * Cross(c->r1, Pt);*/
	//a.angularVel -= a.invRotationalInertia * det(r1, Pt);

	b.vel += b.invMass * Pt;
	//b.angularVel += b.invRotationalInertia * det(r2, Pt);
}
