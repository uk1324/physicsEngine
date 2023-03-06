#include <game/springJoint.hpp>
#include <game/ent.hpp>

auto SpringJoint::preStep(float invDeltaTime) -> void {
	if (!ent.body.isAlive(bodyA) || !ent.body.isAlive(bodyB)) {
		ent.springJoint.destroy(*this);
		return;
	}
	invDt = invDeltaTime;

	auto a = ent.body.get(bodyA);
	auto b = ent.body.get(bodyB);
	if (!a.has_value() || !b.has_value()) {
		return;
	}

	const auto distanceAb = ::distance(a->transform.pos, b->transform.pos);
	const auto error = distanceAb - restLength;
	/*a->vel += (b->transform.pos - a->transform.pos) * error * invDeltaTime * 1.0f;
	b->vel -= (b->transform.pos - a->transform.pos) * error * invDeltaTime * 1.0f;*/
	const auto normal = (b->transform.pos - a->transform.pos).normalized();
	/*const auto k = 500.9f;*/
	const auto k = 50.9f;
	a->vel += normal * error * k / a->mass;
	b->vel -= normal * error * k / b->mass;
}

auto SpringJoint::applyImpluse() -> void {
	auto a = ent.body.get(bodyA);
	auto b = ent.body.get(bodyB);
	if (!a.has_value() || !b.has_value()) {
		return;
	}

	//const auto distanceAb = ::distance(a->transform.pos, b->transform.pos);
	//const auto error = distanceAb - restLength;
	//a->vel += (b->transform.pos - a->transform.pos) * error * invDt * 0.9f;
	//b->vel -= (b->transform.pos - a->transform.pos) * error * invDt * 0.9f;
}

//const auto posOnA = a->transform.pos + localAnchorA * a->transform.rot;
	//const auto posOnB = b->transform.pos + localAnchorB * b->transform.rot;
	//const auto bToA = posOnA - posOnB;
	//const auto distanceAb = bToA.length();
	//const auto distanceToFix = distanceAb - restLength;


	////Vec2 r1{ 0.0f }, r2{ 0.0f };

	//const auto n = bToA.normalized();
	//Vec2 r1 = posOnA - a->transform.pos, r2 = posOnB - b->transform.pos;
	//float rn1 = dot(r1, n);
	//float rn2 = dot(r2, n);
	//float kNormal = a->invMass + b->invMass;
	//kNormal += a->invRotationalInertia * (dot(r1, r1) - rn1 * rn1) + b->invRotationalInertia * (dot(r2, r2) - rn2 * rn2);

	//auto relativeVel = (b->vel + cross(b->angularVel, r2)) - (a->vel + cross(a->angularVel, r1));
	//float vn = dot(relativeVel, n);


	///*float dPn = (-vn * 0.5f) / kNormal;*/
	///*float dPn = (distanceToFix * 0.9f) / kNormal;*/
	///*float dPn = (-vn + distanceToFix / invDt * 0.9f) / kNormal;*/
	///*float dPn = (vn + distanceToFix * 0.001f) / kNormal;*/
	////float dPn = (-vn) / kNormal;
	////float dPn = (0.0f) / kNormal;

	//a->vel -= distanceToFix * invDt * 0.7f * n * a->invMass;
	//b->vel += distanceToFix * invDt * 0.7f * n * b->invMass;

	////a->vel -= dPn * n * a->invMass;
	////b->vel += dPn * n * b->invMass;
	////a->angularVel -= a->invRotationalInertia * cross(r1, dPn * n);
	////b->angularVel += b->invRotationalInertia * cross(r2, dPn * n);