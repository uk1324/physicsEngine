#include <game/distanceJoint.hpp>
#include <game/entitesData.hpp>
#include <math/utils.hpp>

auto DistanceJoint::preStep(Body&, Body&, float invDeltaTime) -> void {
	bias = invDeltaTime;
}

// Don't know what a correct pendulum should look like. Car keys that were left in the ignition were oscillating back and forth for around 3 minutes and if no one interrupted them, they would have continuted for a bit longer. The car keys were connected by a circle so the friction is should probably different from this kind of joint.
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

	const auto n = bToA.normalized();
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

	a.vel -= dPn * n * a.invMass;
	b.vel += dPn * n * b.invMass;

	relativeVel = b.vel - a.vel;
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

	a.vel -= a.invMass * Pt;
	a.angularVel -= a.invRotationalInertia * det(r1, Pt);

	b.vel += b.invMass * Pt;
	b.angularVel += b.invRotationalInertia * det(r2, Pt);
}
