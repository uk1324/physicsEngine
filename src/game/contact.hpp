#pragma once

#include <math/vec2.hpp>

struct Body;

union FeaturePair
{
	struct Edges
	{
		char inEdge1;
		char outEdge1;
		char inEdge2;
		char outEdge2;
	} e;
	int value;
};

struct ContactPoint
{
	ContactPoint() 
		: Pn(0.0f)
		, Pt(0.0f)
		, Pnb(0.0f) {
		separation = 0.0f;
		massNormal = 0.0f;
		bias = 0.0f;
		massTangent = 0.0f;
		feature = { 0 };
	}

	Vec2 position;
	Vec2 normal;
	Vec2 r1, r2;
	float separation;
	float Pn;	// accumulated normal impulse
	float Pt;	// accumulated tangent impulse
	float Pnb;	// accumulated normal impulse for position bias
	float massNormal, massTangent;
	float bias;
	FeaturePair feature;
};

struct ContactKey
{
	ContactKey(Body* b1, Body* b2)
	{
		if (b1 < b2)
		{
			body1 = b1; body2 = b2;
		}
		else
		{
			body1 = b2; body2 = b1;
		}
	}

	auto operator==(const ContactKey& other) const -> bool {
		return body1 == other.body1 && body2 == other.body2;
	}

	Body* body1;
	Body* body2;
};

struct Contact
{
	static constexpr usize MAX_POINTS = 2;

	Contact() {
		coefficientOfFriction = 0.0f;
		numContacts = 0;
	};
	Contact(Body* b1, Body* b2);

	void Update(ContactPoint* contacts, int numContacts);

	void PreStep(float inv_dt);
	void ApplyImpulse();

	ContactPoint contacts[MAX_POINTS];
	int numContacts;

	Body* body1;
	Body* body2;

	// Combined friction
	float coefficientOfFriction;
};

//// This is used by std::set
//inline bool operator< (const ContactKey& a1, const ContactKey& a2)
//{
//	if (a1.body1 < a2.body1)
//		return true;
//
//	if (a1.body1 == a2.body1 && a1.body2 < a2.body2)
//		return true;
//
//	return false;
//}

struct ContactKeyHasher {
	size_t operator()(const ContactKey& x) const {
		return reinterpret_cast<u8>(x.body1) * reinterpret_cast<u8>(x.body2);
	}
};

int Collide(ContactPoint* contacts, Body* body1, Body* body2);