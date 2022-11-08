#include <game/contact.hpp>
#include <game/body.hpp>
#include <game/game2.hpp>
#include <math/mat2.hpp>
#include <math/utils.hpp>
#include <algorithm>

#include <game/collision/collision.hpp>
//#include <

//Collision::Collision(Body* b1, Body* b2)
//{
//	if (b1 < b2) {
//		body1 = b1;
//		body2 = b2;
//	} else {
//		body1 = b2;
//		body2 = b1;
//	}
//
//	numContacts = Collide(contacts, body1, body2);
//
//	coefficientOfFriction = sqrtf(body1->coefficientOfFriction * body2->coefficientOfFriction);
//}

void Collision::Update(ContactPoint* newContacts, int numNewContacts)
{
	ContactPoint mergedContacts[2];

	for (int i = 0; i < numNewContacts; ++i)
	{
		ContactPoint* cNew = newContacts + i;
		int oldContactSameAsNew = -1;
		for (int j = 0; j < numContacts; ++j)
		{
			ContactPoint* cOld = contacts + j;
			if (cNew->feature.value == cOld->feature.value)
			{
				oldContactSameAsNew = j;
				break;
			}
		}

		if (oldContactSameAsNew > -1)
		{
			ContactPoint* c = mergedContacts + i;
			ContactPoint* cOld = contacts + oldContactSameAsNew;
			*c = *cNew;
			if (Game2::warmStarting)
			{
				c->Pn = cOld->Pn;
				c->Pt = cOld->Pt;
				c->Pnb = cOld->Pnb;
			}
			else
			{
				c->Pn = 0.0f;
				c->Pt = 0.0f;
				c->Pnb = 0.0f;
			}
		}
		else
		{
			mergedContacts[i] = newContacts[i];
		}
	}

	for (int i = 0; i < numNewContacts; ++i)
		contacts[i] = mergedContacts[i];

	numContacts = numNewContacts;
}

inline float Cross(const Vec2& a, const Vec2& b)
{
	return a.x * b.y - a.y * b.x;
}

inline Vec2 Cross(const Vec2& a, float s)
{
	return Vec2(s * a.y, -s * a.x);
}

inline Vec2 Cross(float s, const Vec2& a)
{
	return Vec2(-s * a.y, s * a.x);
}

void Collision::PreStep(Body* a, Body* b, float inv_dt)
{
	auto body1 = a;
	auto body2 = b;

	const float k_allowedPenetration = 0.01f;
	float k_biasFactor = Game2::positionCorrection ? 0.2f : 0.0f;

	for (int i = 0; i < numContacts; ++i)
	{
		ContactPoint* c = contacts + i;

		Vec2 r1 = c->position - body1->pos;
		Vec2 r2 = c->position - body2->pos;

		// Precompute normal mass, tangent mass, and bias.
		float rn1 = dot(r1, c->normal);
		float rn2 = dot(r2, c->normal);
		float kNormal = body1->invMass + body2->invMass;
		kNormal += body1->invRotationalInertia * (dot(r1, r1) - rn1 * rn1) + body2->invRotationalInertia * (dot(r2, r2) - rn2 * rn2);
		c->massNormal = 1.0f / kNormal;

		Vec2 tangent = Cross(c->normal, 1.0f);
		float rt1 = dot(r1, tangent);
		float rt2 = dot(r2, tangent);
		float kTangent = body1->invMass + body2->invMass;
		kTangent += body1->invRotationalInertia * (dot(r1, r1) - rt1 * rt1) + body2->invRotationalInertia * (dot(r2, r2) - rt2 * rt2);
		c->massTangent = 1.0f / kTangent;

		c->bias = -k_biasFactor * inv_dt * std::min(0.0f, c->separation + k_allowedPenetration);

		if (Game2::accumulateImpulses)
		{
			// Apply normal + friction impulse
			Vec2 P = c->Pn * c->normal + c->Pt * tangent;

			body1->vel -= body1->invMass * P;
			body1->angularVel -= body1->invRotationalInertia * Cross(r1, P);

			body2->vel += body2->invMass * P;
			body2->angularVel += body2->invRotationalInertia * Cross(r2, P);
		}
	}
}

void Collision::ApplyImpulse(Body* a, Body* b)
{
	Body* b1 = a;
	Body* b2 = b;

	for (int i = 0; i < numContacts; ++i)
	{
		ContactPoint* c = contacts + i;
		c->r1 = c->position - b1->pos;
		c->r2 = c->position - b2->pos;

		// Relative velocity at contact
		Vec2 dv = b2->vel + Cross(b2->angularVel, c->r2) - b1->vel - Cross(b1->angularVel, c->r1);

		// Compute normal impulse
		float vn = dot(dv, c->normal);

		float dPn = c->massNormal * (-vn + c->bias);

		if (Game2::accumulateImpulses)
		{
			// Clamp the accumulated impulse
			float Pn0 = c->Pn;
			c->Pn = std::max(Pn0 + dPn, 0.0f);
			dPn = c->Pn - Pn0;
		}
		else
		{
			dPn = std::max(dPn, 0.0f);
		}

		// Apply contact impulse
		Vec2 Pn = dPn * c->normal;

		b1->vel -= b1->invMass * Pn;
		b1->angularVel -= b1->invRotationalInertia * Cross(c->r1, Pn);

		b2->vel += b2->invMass * Pn;
		b2->angularVel += b2->invRotationalInertia * Cross(c->r2, Pn);

		// Relative velocity at contact
		dv = b2->vel + Cross(b2->angularVel, c->r2) - b1->vel - Cross(b1->angularVel, c->r1);

		Vec2 tangent = Cross(c->normal, 1.0f);
		float vt = dot(dv, tangent);
		float dPt = c->massTangent * (-vt);

		if (Game2::accumulateImpulses)
		{
			// Compute friction impulse
			float maxPt = coefficientOfFriction * c->Pn;

			// Clamp friction
			float oldTangentImpulse = c->Pt;
			c->Pt = std::clamp(oldTangentImpulse + dPt, -maxPt, maxPt);
			dPt = c->Pt - oldTangentImpulse;
		}
		else
		{
			float maxPt = coefficientOfFriction * dPn;
			dPt = std::clamp(dPt, -maxPt, maxPt);
		}

		// Apply contact impulse
		Vec2 Pt = dPt * tangent;

		b1->vel -= b1->invMass * Pt;
		b1->angularVel -= b1->invRotationalInertia * Cross(c->r1, Pt);

		b2->vel += b2->invMass * Pt;
		b2->angularVel += b2->invRotationalInertia * Cross(c->r2, Pt);
	}
}

