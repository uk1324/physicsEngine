#include <game/contact.hpp>
#include <game/body.hpp>
#include <game/game2.hpp>
#include <math/mat2.hpp>
#include <math/utils.hpp>
#include <algorithm>

Contact::Contact(Body* b1, Body* b2)
{
	if (b1 < b2) {
		body1 = b1;
		body2 = b2;
	} else {
		body1 = b2;
		body2 = b1;
	}

	numContacts = Collide(contacts, body1, body2);

	coefficientOfFriction = sqrtf(body1->coefficientOfFriction * body2->coefficientOfFriction);
}

void Contact::Update(ContactPoint* newContacts, int numNewContacts)
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

void Contact::PreStep(float inv_dt)
{
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

void Contact::ApplyImpulse()
{
	Body* b1 = body1;
	Body* b2 = body2;

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

///*
// Box vertex and edge numbering:
//        ^ y
//        |
//        e1
//   v2 ------ v1
//    |        |
// e2 |        | e4  --> x
//    |        |
//   v3 ------ v4
//        e3
//*/
//enum Axis
//{
//	FACE_A_X,
//	FACE_A_Y,
//	FACE_B_X,
//	FACE_B_Y
//};
//
//enum EdgeNumbers
//{
//	NO_EDGE = 0,
//	EDGE1,
//	EDGE2,
//	EDGE3,
//	EDGE4
//};
//
//struct ClipVertex
//{
//	ClipVertex() { fp.value = 0; }
//	Vec2 v;
//	FeaturePair fp;
//};
//
//void Flip(FeaturePair& fp)
//{
//	std::swap(fp.e.inEdge1, fp.e.inEdge2);
//	std::swap(fp.e.outEdge1, fp.e.outEdge2);
//}
//
//int ClipSegmentToLine(ClipVertex vOut[2], ClipVertex vIn[2],
//	const Vec2& normal, float offset, char clipEdge)
//{
//	// Start with no output points
//	int numOut = 0;
//
//	// Calculate the distance of end points to the line
//	float distance0 = dot(normal, vIn[0].v) - offset;
//	float distance1 = dot(normal, vIn[1].v) - offset;
//
//	// If the points are behind the plane
//	if (distance0 <= 0.0f) vOut[numOut++] = vIn[0];
//	if (distance1 <= 0.0f) vOut[numOut++] = vIn[1];
//
//	// If the points are on different sides of the plane
//	if (distance0 * distance1 < 0.0f)
//	{
//		// Find intersection point of edge and plane
//		float interp = distance0 / (distance0 - distance1);
//		vOut[numOut].v = vIn[0].v + interp * (vIn[1].v - vIn[0].v);
//		if (distance0 > 0.0f)
//		{
//			vOut[numOut].fp = vIn[0].fp;
//			vOut[numOut].fp.e.inEdge1 = clipEdge;
//			vOut[numOut].fp.e.inEdge2 = NO_EDGE;
//		}
//		else
//		{
//			vOut[numOut].fp = vIn[1].fp;
//			vOut[numOut].fp.e.outEdge1 = clipEdge;
//			vOut[numOut].fp.e.outEdge2 = NO_EDGE;
//		}
//		++numOut;
//	}
//
//	return numOut;
//}
//
//static void ComputeIncidentEdge(ClipVertex c[2], const Vec2& h, const Vec2& pos, const Mat2& Rot, const Vec2& normal)
//{
//	// The normal is from the reference box. Convert it
//	// to the incident boxe's frame and flip sign.
//	Mat2 RotT = Rot.orthonormalInv();
//	Vec2 n = -(normal * RotT);
//	Vec2 nAbs = normal.applied(abs);
//
//	if (nAbs.x > nAbs.y)
//	{
//		if (sign(n.x) > 0.0f)
//		{
//			c[0].v = Vec2{ h.x, -h.y };
//			c[0].fp.e.inEdge2 = EDGE3;
//			c[0].fp.e.outEdge2 = EDGE4;
//
//			c[1].v = Vec2{ h.x, h.y };
//			c[1].fp.e.inEdge2 = EDGE4;
//			c[1].fp.e.outEdge2 = EDGE1;
//		}
//		else
//		{
//			c[0].v = Vec2{ -h.x, h.y };
//			c[0].fp.e.inEdge2 = EDGE1;
//			c[0].fp.e.outEdge2 = EDGE2;
//
//			c[1].v = Vec2{ -h.x, -h.y };
//			c[1].fp.e.inEdge2 = EDGE2;
//			c[1].fp.e.outEdge2 = EDGE3;
//		}
//	}
//	else
//	{
//		if (sign(n.y) > 0.0f)
//		{
//			c[0].v = Vec2{ h.x, h.y };
//			c[0].fp.e.inEdge2 = EDGE4;
//			c[0].fp.e.outEdge2 = EDGE1;
//
//			c[1].v = Vec2{ -h.x, h.y };
//			c[1].fp.e.inEdge2 = EDGE1;
//			c[1].fp.e.outEdge2 = EDGE2;
//		}
//		else
//		{
//			c[0].v = Vec2{ -h.x, -h.y };
//			c[0].fp.e.inEdge2 = EDGE2;
//			c[0].fp.e.outEdge2 = EDGE3;
//
//			c[1].v = Vec2{ h.x, -h.y };
//			c[1].fp.e.inEdge2 = EDGE3;
//			c[1].fp.e.outEdge2 = EDGE4;
//		}
//	}
//
//	c[0].v = pos + c[0].v * Rot;
//	c[1].v = pos + c[1].v * Rot;
//}
//
//// The normal points from A to B
//int Collide(ContactPoint* contacts, Body* bodyA, Body* bodyB)
//{
//	// Setup
//	Vec2 hA = 0.5f * bodyA->size;
//	Vec2 hB = 0.5f * bodyB->size;
//
//	Vec2 posA = bodyA->pos;
//	Vec2 posB = bodyB->pos;
//
//	const auto RotA = Mat2::rotate(bodyA->orientation), RotB = Mat2::rotate(bodyB->orientation);
//
//	Mat2 RotAT = RotA.orthonormalInv();
//	Mat2 RotBT = RotB.orthonormalInv();
//
//	Vec2 dp = posB - posA;
//	Vec2 dA = dp * RotAT;
//	Vec2 dB = dp * RotBT;
//
//	Mat2 C = RotAT * RotB;
//	auto mAbs = [](const Mat2& m) { return Mat2{ m.x().applied(abs), m.y().applied(abs) }; };
//
//	/*Mat2 absC = Abs(C);*/
//	Mat2 absC = mAbs(C);
//	Mat2 absCT = absC.orthonormalInv();
//
//	// Box A faces
//	Vec2 faceA = dA.applied(abs) - hA - hB * absC;
//	if (faceA.x > 0.0f || faceA.y > 0.0f)
//		return 0;
//
//	// Box B faces
//	Vec2 faceB = dB.applied(abs) - hA * absCT - hB;
//	if (faceB.x > 0.0f || faceB.y > 0.0f)
//		return 0;
//
//	// Find best axis
//	Axis axis;
//	float separation;
//	Vec2 normal;
//
//	// Box A faces
//	axis = FACE_A_X;
//	separation = faceA.x;
//	normal = dA.x > 0.0f ? RotA.x() : -RotA.x();
//
//	const float relativeTol = 0.95f;
//	const float absoluteTol = 0.01f;
//
//	if (faceA.y > relativeTol * separation + absoluteTol * hA.y)
//	{
//		axis = FACE_A_Y;
//		separation = faceA.y;
//		normal = dA.y > 0.0f ? RotA.y() : -RotA.y();
//	}
//
//	// Box B faces
//	if (faceB.x > relativeTol * separation + absoluteTol * hB.x)
//	{
//		axis = FACE_B_X;
//		separation = faceB.x;
//		normal = dB.x > 0.0f ? RotB.x() : -RotB.x();
//	}
//
//	if (faceB.y > relativeTol * separation + absoluteTol * hB.y)
//	{
//		axis = FACE_B_Y;
//		separation = faceB.y;
//		normal = dB.y > 0.0f ? RotB.y() : -RotB.y();
//	}
//
//	// Setup clipping plane data based on the separating axis
//	Vec2 frontNormal, sideNormal;
//	ClipVertex incidentEdge[2];
//	float front, negSide, posSide;
//	char negEdge, posEdge;
//
//	// Compute the clipping lines and the line segment to be clipped.
//	switch (axis)
//	{
//	case FACE_A_X:
//	{
//		frontNormal = normal;
//		front = dot(posA, frontNormal) + hA.x;
//		sideNormal = RotA.y();
//		float side = dot(posA, sideNormal);
//		negSide = -side + hA.y;
//		posSide = side + hA.y;
//		negEdge = EDGE3;
//		posEdge = EDGE1;
//		ComputeIncidentEdge(incidentEdge, hB, posB, RotB, frontNormal);
//	}
//	break;
//
//	case FACE_A_Y:
//	{
//		frontNormal = normal;
//		front = dot(posA, frontNormal) + hA.y;
//		sideNormal = RotA.x();
//		float side = dot(posA, sideNormal);
//		negSide = -side + hA.x;
//		posSide = side + hA.x;
//		negEdge = EDGE2;
//		posEdge = EDGE4;
//		ComputeIncidentEdge(incidentEdge, hB, posB, RotB, frontNormal);
//	}
//	break;
//
//	case FACE_B_X:
//	{
//		frontNormal = -normal;
//		front = dot(posB, frontNormal) + hB.x;
//		sideNormal = RotB.y();
//		float side = dot(posB, sideNormal);
//		negSide = -side + hB.y;
//		posSide = side + hB.y;
//		negEdge = EDGE3;
//		posEdge = EDGE1;
//		ComputeIncidentEdge(incidentEdge, hA, posA, RotA, frontNormal);
//	}
//	break;
//
//	case FACE_B_Y:
//	{
//		frontNormal = -normal;
//		front = dot(posB, frontNormal) + hB.y;
//		sideNormal = RotB.x(); // CHECK
//		float side = dot(posB, sideNormal);
//		negSide = -side + hB.x;
//		posSide = side + hB.x;
//		negEdge = EDGE2;
//		posEdge = EDGE4;
//		ComputeIncidentEdge(incidentEdge, hA, posA, RotA, frontNormal);
//	}
//	break;
//	}
//
//	// clip other face with 5 box planes (1 face plane, 4 edge planes)
//
//	ClipVertex clipPoints1[2];
//	ClipVertex clipPoints2[2];
//	int np;
//
//	// Clip to box side 1
//	np = ClipSegmentToLine(clipPoints1, incidentEdge, -sideNormal, negSide, negEdge);
//
//	if (np < 2)
//		return 0;
//
//	// Clip to negative box side 1
//	np = ClipSegmentToLine(clipPoints2, clipPoints1, sideNormal, posSide, posEdge);
//
//	if (np < 2)
//		return 0;
//
//	// Now clipPoints2 contains the clipping points.
//	// Due to roundoff, it is possible that clipping removes all points.
//
//	int numContacts = 0;
//	for (int i = 0; i < 2; ++i)
//	{
//		float separation = dot(frontNormal, clipPoints2[i].v) - front;
//
//		if (separation <= 0)
//		{
//			contacts[numContacts].separation = separation;
//			contacts[numContacts].normal = normal;
//			// slide contact point onto reference face (easy to cull)
//			contacts[numContacts].position = clipPoints2[i].v - separation * frontNormal;
//			contacts[numContacts].feature = clipPoints2[i].fp;
//			if (axis == FACE_B_X || axis == FACE_B_Y)
//				Flip(contacts[numContacts].feature);
//			++numContacts;
//		}
//	}
//
//	return numContacts;
//}

// Box vertex and edge numbering:
//
//        ^ y
//        |
//        e1
//   v2 ------ v1
//    |        |
// e2 |        | e4  --> x
//    |        |
//   v3 ------ v4
//        e3

//auto mAbs = [](const Mat2& m) { return Mat2{ m.x().applied(abs), m.y().applied(abs) }; };

enum Axis
{
	FACE_A_X,
	FACE_A_Y,
	FACE_B_X,
	FACE_B_Y
};

enum EdgeNumbers
{
	NO_EDGE = 0,
	EDGE1,
	EDGE2,
	EDGE3,
	EDGE4
};

struct ClipVertex
{
	ClipVertex() { fp.value = 0; }
	Vec2 v;
	FeaturePair fp;
};

template<typename T> inline void Swap(T& a, T& b)
{
	T tmp = a;
	a = b;
	b = tmp;
}

void Flip(FeaturePair& fp)
{
	Swap(fp.e.inEdge1, fp.e.inEdge2);
	Swap(fp.e.outEdge1, fp.e.outEdge2);
}

int ClipSegmentToLine(ClipVertex vOut[2], ClipVertex vIn[2],
	const Vec2& normal, float offset, char clipEdge)
{
	// Start with no output points
	int numOut = 0;

	// Calculate the distance of end points to the line
	float distance0 = dot(normal, vIn[0].v) - offset;
	float distance1 = dot(normal, vIn[1].v) - offset;

	// If the points are behind the plane
	if (distance0 <= 0.0f) vOut[numOut++] = vIn[0];
	if (distance1 <= 0.0f) vOut[numOut++] = vIn[1];

	// If the points are on different sides of the plane
	if (distance0 * distance1 < 0.0f)
	{
		// Find intersection point of edge and plane
		float interp = distance0 / (distance0 - distance1);
		vOut[numOut].v = vIn[0].v + interp * (vIn[1].v - vIn[0].v);
		if (distance0 > 0.0f)
		{
			vOut[numOut].fp = vIn[0].fp;
			vOut[numOut].fp.e.inEdge1 = clipEdge;
			vOut[numOut].fp.e.inEdge2 = NO_EDGE;
		}
		else
		{
			vOut[numOut].fp = vIn[1].fp;
			vOut[numOut].fp.e.outEdge1 = clipEdge;
			vOut[numOut].fp.e.outEdge2 = NO_EDGE;
		}
		++numOut;
	}

	return numOut;
}

inline Vec2 Abs(const Vec2& a)
{
	return Vec2(fabsf(a.x), fabsf(a.y));
}

inline float Sign(float x)
{
	return x < 0.0f ? -1.0f : 1.0f;
}

inline Mat2 Abs(const Mat2& A)
{
	return Mat2(Abs(A.x()), Abs(A.y()));
}

static void ComputeIncidentEdge(ClipVertex c[2], const Vec2& h, const Vec2& pos,
	const Mat2& Rot, const Vec2& normal)
{
	// The normal is from the reference box. Convert it
	// to the incident boxe's frame and flip sign.
	Mat2 RotT = Rot.transposed();
	Vec2 n = -(normal * RotT);
	Vec2 nAbs = Abs(n);

	if (nAbs.x > nAbs.y)
	{
		if (Sign(n.x) > 0.0f)
		{
			c[0].v = Vec2(h.x, -h.y);
			c[0].fp.e.inEdge2 = EDGE3;
			c[0].fp.e.outEdge2 = EDGE4;

			c[1].v = Vec2(h.x, h.y);
			c[1].fp.e.inEdge2 = EDGE4;
			c[1].fp.e.outEdge2 = EDGE1;
		}
		else
		{
			c[0].v = Vec2(-h.x, h.y);
			c[0].fp.e.inEdge2 = EDGE1;
			c[0].fp.e.outEdge2 = EDGE2;

			c[1].v = Vec2(-h.x, -h.y);
			c[1].fp.e.inEdge2 = EDGE2;
			c[1].fp.e.outEdge2 = EDGE3;
		}
	}
	else
	{
		if (Sign(n.y) > 0.0f)
		{
			c[0].v = Vec2(h.x, h.y);
			c[0].fp.e.inEdge2 = EDGE4;
			c[0].fp.e.outEdge2 = EDGE1;

			c[1].v = Vec2(-h.x, h.y);
			c[1].fp.e.inEdge2 = EDGE1;
			c[1].fp.e.outEdge2 = EDGE2;
		}
		else
		{
			c[0].v = Vec2(-h.x, -h.y);
			c[0].fp.e.inEdge2 = EDGE2;
			c[0].fp.e.outEdge2 = EDGE3;

			c[1].v = Vec2(h.x, -h.y);
			c[1].fp.e.inEdge2 = EDGE3;
			c[1].fp.e.outEdge2 = EDGE4;
		}
	}

	c[0].v = pos + c[0].v * Rot;
	c[1].v = pos + c[1].v * Rot;
}

// The normal points from A to B
int Collide(ContactPoint* contacts, Body* bodyA, Body* bodyB)
{
	// Setup
	Vec2 hA = 0.5f * bodyA->size;
	Vec2 hB = 0.5f * bodyB->size;

	Vec2 posA = bodyA->pos;
	Vec2 posB = bodyB->pos;

	Mat2 RotA = Mat2::rotate(bodyA->orientation), RotB = Mat2::rotate(bodyB->orientation);

	Mat2 RotAT = RotA.transposed();
	Mat2 RotBT = RotB.transposed();

	Vec2 dp = posB - posA;
	Vec2 dA = dp * RotAT;
	Vec2 dB = dp * RotBT;

	Mat2 C = RotAT * RotB;
	Mat2 absC = Abs(C);
	Mat2 absCT = absC.transposed();

	// Box A faces
	Vec2 faceA = Abs(dA) - hA - hB * absC;
	if (faceA.x > 0.0f || faceA.y > 0.0f)
		return 0;

	// Box B faces
	Vec2 faceB = Abs(dB) - hA * absCT - hB;
	if (faceB.x > 0.0f || faceB.y > 0.0f)
		return 0;

	// Find best axis
	Axis axis;
	float separation;
	Vec2 normal;

	// Box A faces
	axis = FACE_A_X;
	separation = faceA.x;
	normal = dA.x > 0.0f ? RotA.x() : -RotA.x();

	const float relativeTol = 0.95f;
	const float absoluteTol = 0.01f;

	if (faceA.y > relativeTol * separation + absoluteTol * hA.y)
	{
		axis = FACE_A_Y;
		separation = faceA.y;
		normal = dA.y > 0.0f ? RotA.y() : -RotA.y();
	}

	// Box B faces
	if (faceB.x > relativeTol * separation + absoluteTol * hB.x)
	{
		axis = FACE_B_X;
		separation = faceB.x;
		normal = dB.x > 0.0f ? RotB.x() : -RotB.x();
	}

	if (faceB.y > relativeTol * separation + absoluteTol * hB.y)
	{
		axis = FACE_B_Y;
		separation = faceB.y;
		normal = dB.y > 0.0f ? RotB.y() : -RotB.y();
	}

	// Setup clipping plane data based on the separating axis
	Vec2 frontNormal, sideNormal;
	ClipVertex incidentEdge[2];
	float front, negSide, posSide;
	char negEdge, posEdge;

	// Compute the clipping lines and the line segment to be clipped.
	switch (axis)
	{
	case FACE_A_X:
	{
		frontNormal = normal;
		front = dot(posA, frontNormal) + hA.x;
		sideNormal = RotA.y();
		float side = dot(posA, sideNormal);
		negSide = -side + hA.y;
		posSide = side + hA.y;
		negEdge = EDGE3;
		posEdge = EDGE1;
		ComputeIncidentEdge(incidentEdge, hB, posB, RotB, frontNormal);
	}
	break;

	case FACE_A_Y:
	{
		frontNormal = normal;
		front = dot(posA, frontNormal) + hA.y;
		sideNormal = RotA.x();
		float side = dot(posA, sideNormal);
		negSide = -side + hA.x;
		posSide = side + hA.x;
		negEdge = EDGE2;
		posEdge = EDGE4;
		ComputeIncidentEdge(incidentEdge, hB, posB, RotB, frontNormal);
	}
	break;

	case FACE_B_X:
	{
		frontNormal = -normal;
		front = dot(posB, frontNormal) + hB.x;
		sideNormal = RotB.y();
		float side = dot(posB, sideNormal);
		negSide = -side + hB.y;
		posSide = side + hB.y;
		negEdge = EDGE3;
		posEdge = EDGE1;
		ComputeIncidentEdge(incidentEdge, hA, posA, RotA, frontNormal);
	}
	break;

	case FACE_B_Y:
	{
		frontNormal = -normal;
		front = dot(posB, frontNormal) + hB.y;
		sideNormal = RotB.x();
		float side = dot(posB, sideNormal);
		negSide = -side + hB.x;
		posSide = side + hB.x;
		negEdge = EDGE2;
		posEdge = EDGE4;
		ComputeIncidentEdge(incidentEdge, hA, posA, RotA, frontNormal);
	}
	break;
	}

	// clip other face with 5 box planes (1 face plane, 4 edge planes)

	ClipVertex clipPoints1[2];
	ClipVertex clipPoints2[2];
	int np;

	// Clip to box side 1
	np = ClipSegmentToLine(clipPoints1, incidentEdge, -sideNormal, negSide, negEdge);

	if (np < 2)
		return 0;

	// Clip to negative box side 1
	np = ClipSegmentToLine(clipPoints2, clipPoints1, sideNormal, posSide, posEdge);

	if (np < 2)
		return 0;

	// Now clipPoints2 contains the clipping points.
	// Due to roundoff, it is possible that clipping removes all points.

	int numContacts = 0;
	for (int i = 0; i < 2; ++i)
	{
		float separation = dot(frontNormal, clipPoints2[i].v) - front;

		if (separation <= 0)
		{
			contacts[numContacts].separation = separation;
			contacts[numContacts].normal = normal;
			// slide contact point onto reference face (easy to cull)
			contacts[numContacts].position = clipPoints2[i].v - separation * frontNormal;
			contacts[numContacts].feature = clipPoints2[i].fp;
			if (axis == FACE_B_X || axis == FACE_B_Y)
				Flip(contacts[numContacts].feature);
			++numContacts;
		}
	}

	return numContacts;
}