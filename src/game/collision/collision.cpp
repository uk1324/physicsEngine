#include <game/collision/collision.hpp>
#include <game/game.hpp>
#include <math/mat2.hpp>
#include <math/utils.hpp>
#include <algorithm>

auto Collision::update(ContactPoint* newContacts, i32 newContactCount) -> void {
	ContactPoint mergedContacts[2];

	for (i32 i = 0; i < newContactCount; i++)
	{
		ContactPoint* cNew = newContacts + i;
		i32 oldContactSameAsNew = -1;
		for (int j = 0; j < contactCount; j++) {
			ContactPoint* cOld = contacts + j;
			if (cNew->feature.value == cOld->feature.value) {
				oldContactSameAsNew = j;
				break;
			}
		}

		if (oldContactSameAsNew > -1) {
			ContactPoint* c = mergedContacts + i;
			ContactPoint* old = contacts + oldContactSameAsNew;
			*c = *cNew;
			if (Game::reusePreviousFrameContactAccumulators) {
				c->accumulatedNormalImpluse = old->accumulatedNormalImpluse;
				c->accumulatedTangentImpulse = old->accumulatedTangentImpulse;
			} else {
				c->accumulatedNormalImpluse = 0.0f;
				c->accumulatedTangentImpulse = 0.0f;
			}
		} else {
			mergedContacts[i] = newContacts[i];
		}
	}

	for (i32 i = 0; i < newContactCount; ++i)
		contacts[i] = mergedContacts[i];

	contactCount = newContactCount;
}

auto Collision::preStep(Body* a, Body* b, float invDeltaTime) -> void {
	for (int i = 0; i < contactCount; ++i) {
		ContactPoint* c = contacts + i;
		
		// n = normal
		// cA = contactPosOnA
		// cB = contactPosOnB = contactPosOnA + n
		// C(x) = dot(cA - cB, n)
		// C(x) = dot(pA + rA - (pB + rB), n)
		// C'(x) = dot(velA - velB, n)
		// C'(x) = dot((vA + aA * rA) - (vB + aB * rB), n)
		// JV + b = 0

		Vec2 r1 = c->position - a->pos;
		Vec2 r2 = c->position - b->pos;

		// Precompute normal mass, tangent mass, and bias.
		float rn1 = dot(r1, c->normal);
		float rn2 = dot(r2, c->normal);
		float kNormal = a->invMass + b->invMass;
		kNormal += a->invRotationalInertia * (dot(r1, r1) - rn1 * rn1) + b->invRotationalInertia * (dot(r2, r2) - rn2 * rn2);
		c->invNormalEffectiveMass = 1.0f / kNormal;

		Vec2 tangent = cross(c->normal, 1.0f);
		float rt1 = dot(r1, tangent);
		float rt2 = dot(r2, tangent);
		float kTangent = a->invMass + b->invMass;
		kTangent += a->invRotationalInertia * (dot(r1, r1) - rt1 * rt1) + b->invRotationalInertia * (dot(r2, r2) - rt2 * rt2);
		c->invTangentEffectiveMass = 1.0f / kTangent;

		// Baumgarte stabilization bias.
		// The velocity needed to solve the penetration in a single frame is when bias = 1.
		// bias = factor * (penetration / deltaTime)
		// bias = [scalar] * [velocity]
		const auto biasFactor = 0.2f;
		// Decreasing the allowed penetration makes things more bouncy.
		/*const auto k_allowedPenetration = 0.01f;*/
		// Some penetration is allowed to decrease jitter, especially when multiple objects are stacked on top of eachother. This is sometimes called slop.
		const auto ALLOWED_PENETRATION = 0.01f;

		auto relativeVelAtContact = (b->vel + cross(b->angularVel, r2)) - (a->vel + cross(a->angularVel, r1));

		c->bias = -biasFactor * invDeltaTime * std::min(0.0f, c->penetrationDepth + ALLOWED_PENETRATION);
		/*c->bias = std::max(c->bias, -dot(b->vel - a->vel, c->normal)) * 0.5f;*/
		/*c->bias = std::max(c->bias, -dot(relativeVelAtContact, c->normal) * 0.2f);
		c->bias = std::max(0.0f, c->bias);*/


		if (Game::accumulateImpulses)
		{
			// Apply normal + friction impulse
			Vec2 P = c->accumulatedNormalImpluse * c->normal + c->accumulatedTangentImpulse * tangent;

			a->vel -= a->invMass * P;
			a->angularVel -= a->invRotationalInertia * cross(r1, P);

			b->vel += b->invMass * P;
			b->angularVel += b->invRotationalInertia * cross(r2, P);
		}
	}
}

auto Collision::applyImpulse(Body& a, Body& b) -> void {
	for (i32 i = 0; i < contactCount; i++) {
		auto& contact = contacts[i];

		const auto r1 = contact.position - a.pos;
		const auto r2 = contact.position - b.pos;
		auto relativeVelAtContact = (b.vel + cross(b.angularVel, r2)) - (a.vel + cross(a.angularVel, r1));

		// Compute normal impulse
		// You apply the bias if the objects are moving slowly (relative velocity is small) otherwise this bias is the restitution if they are moving fast (large relative velocity). Also, make sure you only calculate this bias once for every time step, and  not for every iteration of impulses.
		const auto velocityAlongNormal = dot(relativeVelAtContact, contact.normal);

		auto normalImpulse = contact.invNormalEffectiveMass * (-velocityAlongNormal + contact.bias);

		if (Game::accumulateImpulses) {
			const auto oldNormalImpluse = contact.accumulatedNormalImpluse;
			contact.accumulatedNormalImpluse = std::max(oldNormalImpluse + normalImpulse, 0.0f);
			normalImpulse = contact.accumulatedNormalImpluse - oldNormalImpluse;
		} else {
			normalImpulse = std::max(normalImpulse, 0.0f);
		}

		const auto impulseNormal = normalImpulse * contact.normal;

		a.vel -= a.invMass * impulseNormal;
		a.angularVel -= a.invRotationalInertia * cross(r1, impulseNormal);

		b.vel += b.invMass * impulseNormal;
		b.angularVel += b.invRotationalInertia * cross(r2, impulseNormal);

		relativeVelAtContact = b.vel + cross(b.angularVel, r2) - a.vel - cross(a.angularVel, r1);

		Vec2 tangent = cross(contact.normal, 1.0f);
		float velocityAlongTangent = dot(relativeVelAtContact, tangent);
		float tangentImpulse = contact.invTangentEffectiveMass * (-velocityAlongTangent);

		if (Game::accumulateImpulses) {
			const auto max = coefficientOfFriction * contact.accumulatedNormalImpluse;
			const auto oldTangentImpulse = contact.accumulatedTangentImpulse;
			contact.accumulatedTangentImpulse = std::clamp(oldTangentImpulse + tangentImpulse, -max, max);
			tangentImpulse = contact.accumulatedTangentImpulse - oldTangentImpulse;
		} else {
			float max = coefficientOfFriction * normalImpulse;
			tangentImpulse = std::clamp(tangentImpulse, -max, max);
		}

		Vec2 impluseTangent = tangentImpulse * tangent;

		a.vel -= a.invMass * impluseTangent;
		a.angularVel -= a.invRotationalInertia * cross(r1, impluseTangent);

		b.vel += b.invMass * impluseTangent;
		b.angularVel += b.invRotationalInertia * cross(r2, impluseTangent);
	}
}

BoxCollider::BoxCollider(const BoxColliderEditor& box) : BoxColliderEditor{ box } {}

auto BoxCollider::massInfo(float density) const -> MassInfo {
	const auto mass = size.x * size.y * density;
	return MassInfo{
		.mass = mass,
		.rotationalInertia = mass * pow(size.x, 2.0f) * pow(size.y, 2.0f) / 12.0f
	};
}

auto BoxCollider::aabb(Vec2 pos, float orientation) const -> Aabb {
	const auto corners = getCorners(pos, orientation);
	return Aabb::fromPoints(Span{ corners.data(), corners.size() });
}

auto BoxCollider::getCorners(Vec2 pos, float orientation) const -> std::array<Vec2, 4> {
	const auto normals = Mat2::rotate(orientation);
	const auto cornerDir = normals.x() * (size.x / 2.0f) + normals.y() * (size.y / 2.0f);
	return { pos + cornerDir, pos + cornerDir - normals.x() * size.x, pos - cornerDir, pos - cornerDir + normals.x() * size.x };
}

auto BoxCollider::getEdges(Vec2 pos, float orientation) const -> std::array<LineSegment, 4> {
	// @Performance Could do this by translating line segments.
	/*const auto normals = Mat2::rotate(orientation);
	LineSegment{ Line{ normals.x(),  } }*/
	const auto corners = getCorners(pos, orientation);
	return {
		LineSegment{ corners[0], corners[1] },
		LineSegment{ corners[1], corners[2] },
		LineSegment{ corners[2], corners[3] },
		LineSegment{ corners[3], corners[0] }
	};
}

CircleCollider::CircleCollider(const CircleColliderEditor& circle) : CircleColliderEditor{ circle } {}

auto CircleCollider::massInfo(float density) const -> MassInfo {
	const auto mass = TAU<float> * pow(radius, 2.0f) * density;
	return MassInfo{
		.mass = mass,
		.rotationalInertia = mass * pow(radius, 2.0f) / 2.0f
	};
}

auto CircleCollider::aabb(Vec2 pos, float) const -> Aabb {
	return Aabb{ pos + Vec2{ -radius }, pos + Vec2{ radius } };
}

// If there is a std::visit error check if the function is const.

auto massInfo(const Collider& collider, float density) -> MassInfo {
	return std::visit(
		[&density](auto&& collider) -> MassInfo {
			return collider.massInfo(density);
		},
		collider
	);
}

auto aabb(const Collider& collider, Vec2 pos, float orientation) -> Aabb {
	return std::visit(
		[&pos, &orientation](auto&& collider) -> Aabb {
			return collider.aabb(pos, orientation);
		},
		collider
	);
}

auto collide(Vec2 aPos, float aOrientation, const Collider& aCollider, Vec2 bPos, float bOrientation, const Collider& bCollider) -> std::optional<Collision>
{
#define GET(collider, name, type) const auto name = std::get_if<type>(&collider); name != nullptr

	if (GET(aCollider, aBox, BoxCollider)) {
		if (GET(bCollider, bBox, BoxCollider)) {
			return collide(aPos, aOrientation, *aBox, bPos, bOrientation, *bBox);
		} else if (GET(bCollider, bCircle, CircleCollider)) {
			return collide(aPos, aOrientation, *aBox, bPos, bOrientation, *bCircle);
		}
	} else if (GET(aCollider, aCircle, CircleCollider)) {
		if (GET(bCollider, bCircle, CircleCollider)) {
			return collide(aPos, aOrientation, *aCircle, bPos, bOrientation, *bCircle);
		}
		if (GET(bCollider, bBox, BoxCollider)) {
			auto collision = collide(bPos, bOrientation, *bBox, aPos, aOrientation, *aCircle);
			if (collision.has_value()) {
				for (auto& contact : collision->contacts)
					contact.normal = -contact.normal;
			}
			return collision;
		}
	}
	// Remember to flip the normal if the arguments are in the opposite order.

	ASSERT_NOT_REACHED();
	return std::nullopt;
}

auto contains(Vec2 point, Vec2 pos, float orientation, const Collider& collider) -> bool {
	return std::visit(
		[&](auto&& collider) -> bool {
			return contains(point, pos, orientation, collider);
		},
		collider
	);
}

/*
 Box vertex and edge numbering:
		^ y
		|
		e1
   v2 ------ v1
	|        |
 e2 |        | e4  --> x
	|        |
   v3 ------ v4
		e3
*/

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

//struct ConvexPolygon {
//	std::vector<Vec2> verts;
//	std::vector<Vec2> normals;
//
//	auto calculateNormals() -> void {
//		if (verts.size() < 3) {
//			ASSERT_NOT_REACHED();
//			return;
//		}
//
//		for (usize i = 0; i < verts.size() - 1; i++) {
//			normals.push_back((verts[i + 1] - verts[i]).rotBy90deg().normalized());
//		}
//		normals.push_back((verts[0] - verts.back()).rotBy90deg().normalized());
//	}
//};
//
//struct Separation {
//	float separation;
//	i32 edgeIndex;
//};
//
//static auto minSeparation(const ConvexPolygon& a, Vec2 aPos, const Mat2& aRot, const ConvexPolygon& b, Vec2 bPos, const Mat2& bRot) -> std::optional<Separation> {
//	auto maxSeparation = -std::numeric_limits<float>::infinity();
//	i32 maxSeparationNormal = 0;
//
//	for (i32 nI = 0; nI < a.normals.size(); nI++) {
//		auto n = a.normals[nI];
//		n *= aRot;
//
//		auto minA = std::numeric_limits<float>::infinity();
//		auto maxA = -minA;
//		auto minB = minA, maxB = maxA;
//
//		for (auto v : a.verts) {
//			v = v * aRot + aPos;
//			const auto d = dot(n, v);
//			if (d < minA) {
//				minA = d;
//			}
//			if (d > maxA) {
//				maxA = d;
//			}
//		}
//
//		for (auto v : b.verts) {
//			v = v * bRot + bPos;
//			const auto d = dot(n, v);
//			if (d < minB) {
//				minB = d;
//			}
//			if (d > maxB) {
//				maxB = d;
//			}
//		}
//
//		if (minA <= maxB && maxA >= minB) {
//			auto separation = minB - maxA;
//			if (separation > maxSeparation) {
//				maxSeparation = separation;
//				maxSeparationNormal = nI;
//			}
//		} else {
//			auto min = a.normals[nI] * aRot * 0.1f;
//			auto edge = (a.verts[nI] + a.verts[(nI + 1) % a.verts.size()]) / 2.0f * aRot * 0.1f + aPos;
//			return std::nullopt;
//		}
//	}
//
//	return Separation{
//		maxSeparation,
//		maxSeparationNormal
//	};
//}
//
////struct Transform {
////
////};
//
//struct Contact {
//	Vec2 pos;
//	float penetration;
//	i32 edge1, edge2;
//};
//
//static auto collide(const ConvexPolygon& a, Vec2 aPos, float aOrientation, const ConvexPolygon& b, Vec2 bPos, float bOrientation) -> std::vector<Contact> {
//	Vec2 min;
//	Vec2 edge;
//
//	std::vector<Contact> c;
//
//	auto aS = minSeparation(a, aPos, Mat2::rotate(aOrientation), b, bPos, Mat2::rotate(bOrientation));
//	if (!aS.has_value())
//		return c;
//	auto bS = minSeparation(b, bPos, Mat2::rotate(bOrientation), a, aPos, Mat2::rotate(aOrientation));
//	if (!bS.has_value())
//		return c;
//
//	const ConvexPolygon* reference;
//	Mat3x2 referenceTransform;
//	const ConvexPolygon* incident;
//	Mat3x2 incidentTransform;
//	Mat2 incidentRot;
//	Vec2 normal;
//
//	Vec2 edgeStart;
//	Vec2 edgeEnd;
//	i32 referenceEdgeIndex;
//
//	Vec2 referenceFaceMidPoint;
//	if (aS->separation > bS->separation) {
//		reference = &a;
//		incident = &b;
//		normal = a.normals[aS->edgeIndex] * Mat2::rotate(aOrientation) * aS->separation;
//		referenceFaceMidPoint = (a.verts[aS->edgeIndex] + a.verts[(aS->edgeIndex + 1) % a.verts.size()]) / 2.0f * Mat2::rotate(aOrientation) + aPos;
//		/*min = a.normals[aS->edgeIndex] * Mat2::rotate(aOrientation) * aS->separation;
//		edge = (a.verts[aS->edgeIndex] + a.verts[(aS->edgeIndex + 1) % a.verts.size()]) / 2.0f * Mat2::rotate(aOrientation) + aPos;*/
//		referenceTransform = Mat3x2::rotate(aOrientation) * Mat3x2::translate(aPos);
//		incidentTransform = Mat3x2::rotate(bOrientation) * Mat3x2::translate(bPos);
//		incidentRot = Mat2::rotate(bOrientation);
//		edgeStart = a.verts[aS->edgeIndex] * referenceTransform;
//		edgeEnd = a.verts[(aS->edgeIndex + 1) % a.verts.size()] * referenceTransform;
//		referenceEdgeIndex = aS->edgeIndex;
//	} else {
//		reference = &b;
//		incident = &a;
//		normal = b.normals[bS->edgeIndex] * Mat2::rotate(bOrientation) * bS->separation;
//		referenceFaceMidPoint = (b.verts[bS->edgeIndex] + b.verts[(bS->edgeIndex + 1) % b.verts.size()]) / 2.0f * Mat2::rotate(bOrientation) + bPos;
//		/*min = b.normals[bS->edgeIndex] * Mat2::rotate(bOrientation) * bS->separation;
//		edge = (b.verts[bS->edgeIndex] + b.verts[(bS->edgeIndex + 1) % b.verts.size()]) / 2.0f * Mat2::rotate(bOrientation) + bPos;*/
//		referenceTransform = Mat3x2::rotate(bOrientation) * Mat3x2::translate(bPos);
//		incidentTransform = Mat3x2::rotate(aOrientation) * Mat3x2::translate(aPos);
//		incidentRot = Mat2::rotate(aOrientation);
//		edgeStart = b.verts[bS->edgeIndex] * referenceTransform;
//		edgeEnd = b.verts[(bS->edgeIndex + 1) % b.verts.size()] * referenceTransform;
//		referenceEdgeIndex = bS->edgeIndex;
//	}
//
//	i32 furthestPointOfIndidentInsideReference = 0;
//	auto maxDistance = -std::numeric_limits<float>::infinity();
//	for (usize i = 0; i < incident->verts.size(); i++) {
//		auto d = dot(normal, incident->verts[i] * incidentTransform);
//		if (d > maxDistance) {
//			maxDistance = d;
//			furthestPointOfIndidentInsideReference = i;
//		}
//	}
//
//	auto getEdges = [](const ConvexPolygon* p, u32 i, const Mat3x2& t) -> std::pair<Vec2, Vec2> {
//		return { p->verts[i] * t, p->verts[(i + 1) % p->verts.size()] * t };
//	};
//
//
//	auto face0Index = furthestPointOfIndidentInsideReference;
//	auto face1Index = (furthestPointOfIndidentInsideReference - 1);
//	if (face1Index < 0) {
//		face1Index = incident->normals.size() - 1;
//	}
//	auto face0 = incident->normals[face0Index] * incidentRot;
//	auto face1 = incident->normals[face1Index] * incidentRot;
//
//	i32 face;
//	if (dot(normal, face0) > dot(normal, face1)) {
//		face = face0Index;
//	} else {
//		face = face1Index;
//	}
//
//	auto incidentEdges = getEdges(incident, face, incidentTransform);
//
//	Line lineA{ edgeStart, edgeStart + normal };
//	Line lineB{ edgeEnd, edgeEnd - normal };
//
//	auto clipPoints = [](const std::pair<Vec2, Vec2>& points, const Line& line) -> std::pair<Vec2, Vec2> {
//		auto clipPoint = [](Vec2 p, const Line& line, const Line& edgeLine) -> Vec2 {
//			auto distanceFromLine = signedDistance(line, p);
//			if (distanceFromLine > 0.0f) {
//				auto x = line.intersection(edgeLine);
//				if (!x.has_value()) {
//					//Debug::drawPoint(Vec2{ 0.0f }, Vec3::GREEN);
//				}
//
//				return *x;
//			}
//			return p;
//		};
//
//		Line edgeLine{ points.first, points.second };
//
//		return {
//			clipPoint(points.first, line, edgeLine),
//			clipPoint(points.second, line, edgeLine)
//		};
//	};
//
//	auto cliped = clipPoints(incidentEdges, lineA);
//	cliped = clipPoints(cliped, lineB);
//
//	std::vector<Contact> points;
//	Line incidentFaceLine{ edgeStart, edgeEnd };
//	auto d = signedDistance(incidentFaceLine, cliped.first);
//	if (d >= 0.0f) {
//		points.push_back({ cliped.first, d, referenceEdgeIndex, face });
//	}
//
//	d = signedDistance(incidentFaceLine, cliped.second);
//	if (d >= 0.0f) {
//		points.push_back({ cliped.second, d, referenceEdgeIndex, face });
//	}
//}

auto collide(Vec2 aPos, float aOrientation, const BoxCollider& aBox, Vec2 bPos, float bOrientation, const BoxCollider& bBox) -> std::optional<Collision> {
	// Setup
	Vec2 halfSizeA = 0.5f * aBox.size;
	Vec2 halfSizeB = 0.5f * bBox.size;

	Vec2 posA = aPos;
	Vec2 posB = bPos;

	Mat2 rotA = Mat2::rotate(aOrientation), rotB = Mat2::rotate(bOrientation);
	Mat2 rotAInv = rotA.transposed(), rotBInv = rotB.transposed();

	Vec2 aPosToBPos = posB - posA;
	Vec2 aPosToBPosInAObjectSpace = aPosToBPos * rotAInv;
	Vec2 aPosToBPosInBObjectSpace = aPosToBPos * rotBInv;

	Mat2 C = rotAInv * rotB;
	Mat2 absC = Abs(C);
	Mat2 absCT = absC.transposed();

	// Box A faces
	Vec2 faceA = Abs(aPosToBPosInAObjectSpace) - halfSizeA - halfSizeB * absC;
	if (faceA.x > 0.0f || faceA.y > 0.0f)
		return std::nullopt;

	// Box B faces
	Vec2 faceB = Abs(aPosToBPosInBObjectSpace) - halfSizeA * absCT - halfSizeB;
	if (faceB.x > 0.0f || faceB.y > 0.0f)
		return std::nullopt;

	// Find best axis
	Axis axis;
	float penetrationDepth;
	Vec2 normal;

	// Box A faces
	axis = FACE_A_X;
	penetrationDepth = faceA.x;
	normal = aPosToBPosInAObjectSpace.x > 0.0f ? rotA.x() : -rotA.x();

	const float relativeTol = 0.95f;
	const float absoluteTol = 0.01f;

	if (faceA.y > relativeTol * penetrationDepth + absoluteTol * halfSizeA.y)
	{
		axis = FACE_A_Y;
		penetrationDepth = faceA.y;
		normal = aPosToBPosInAObjectSpace.y > 0.0f ? rotA.y() : -rotA.y();
	}

	// Box B faces
	if (faceB.x > relativeTol * penetrationDepth + absoluteTol * halfSizeB.x)
	{
		axis = FACE_B_X;
		penetrationDepth = faceB.x;
		normal = aPosToBPosInBObjectSpace.x > 0.0f ? rotB.x() : -rotB.x();
	}

	if (faceB.y > relativeTol * penetrationDepth + absoluteTol * halfSizeB.y)
	{
		axis = FACE_B_Y;
		penetrationDepth = faceB.y;
		normal = aPosToBPosInBObjectSpace.y > 0.0f ? rotB.y() : -rotB.y();
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
		front = dot(posA, frontNormal) + halfSizeA.x;
		sideNormal = rotA.y();
		float side = dot(posA, sideNormal);
		negSide = -side + halfSizeA.y;
		posSide = side + halfSizeA.y;
		negEdge = EDGE3;
		posEdge = EDGE1;
		ComputeIncidentEdge(incidentEdge, halfSizeB, posB, rotB, frontNormal);
	}
	break;

	case FACE_A_Y:
	{
		frontNormal = normal;
		front = dot(posA, frontNormal) + halfSizeA.y;
		sideNormal = rotA.x();
		float side = dot(posA, sideNormal);
		negSide = -side + halfSizeA.x;
		posSide = side + halfSizeA.x;
		negEdge = EDGE2;
		posEdge = EDGE4;
		ComputeIncidentEdge(incidentEdge, halfSizeB, posB, rotB, frontNormal);
	}
	break;

	case FACE_B_X:
	{
		frontNormal = -normal;
		front = dot(posB, frontNormal) + halfSizeB.x;
		sideNormal = rotB.y();
		float side = dot(posB, sideNormal);
		negSide = -side + halfSizeB.y;
		posSide = side + halfSizeB.y;
		negEdge = EDGE3;
		posEdge = EDGE1;
		ComputeIncidentEdge(incidentEdge, halfSizeA, posA, rotA, frontNormal);
	}
	break;

	case FACE_B_Y:
	{
		frontNormal = -normal;
		front = dot(posB, frontNormal) + halfSizeB.y;
		sideNormal = rotB.x();
		float side = dot(posB, sideNormal);
		negSide = -side + halfSizeB.x;
		posSide = side + halfSizeB.x;
		negEdge = EDGE2;
		posEdge = EDGE4;
		ComputeIncidentEdge(incidentEdge, halfSizeA, posA, rotA, frontNormal);
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
		return std::nullopt;

	// Clip to negative box side 1
	np = ClipSegmentToLine(clipPoints2, clipPoints1, sideNormal, posSide, posEdge);

	if (np < 2)
		return std::nullopt;

	// Now clipPoints2 contains the clipping points.
	// Due to roundoff, it is possible that clipping removes all points.

	Collision collision;

	int numContacts = 0;
	for (int i = 0; i < 2; ++i)
	{
		float penetrationDepth = dot(frontNormal, clipPoints2[i].v) - front;

		if (penetrationDepth <= 0)
		{
			collision.contacts[numContacts].penetrationDepth = penetrationDepth;
			collision.contacts[numContacts].normal = normal;
			// slide contact point onto reference face (easy to cull)
			collision.contacts[numContacts].position = clipPoints2[i].v - penetrationDepth * frontNormal;
			collision.contacts[numContacts].feature = clipPoints2[i].fp;
			if (axis == FACE_B_X || axis == FACE_B_Y)
				Flip(collision.contacts[numContacts].feature);
			++numContacts;
		}
	}

	collision.contactCount = numContacts;
	return collision;
}

// Another way to do this that would work is to do the same thing if the center is inside the box else calculate the seperation vector by calculating the distance from the sides of the box to the center. The length would then be circle.radius - v.length().
// Hacky but working code.
//Vec2 distanceFromBoxSideToCircleCenter = {
//	std::max(0.0f, abs(alongX) - boxHalfSize.x) * -sign(alongX),
//	std::max(0.0f, abs(alongY) - boxHalfSize.y) * -sign(alongY)
//};
auto collide(Vec2 boxPos, float boxOrientation, const BoxCollider& box, Vec2 circlePos, float, const CircleCollider& circle) -> std::optional<Collision> {
	const auto boxRotationInverse = Mat2::rotate(-boxOrientation);
	const auto boxHalfSize = box.size / 2.0f;
	Collision collision;

	// Could also do calculations in the box's space by using the dot(normals.<x, y>, circlePos) and then convert to by multiplting by the result's components by the basis, but I think that would use more calculations.
	const auto circlePosInBoxSpace = (circlePos - boxPos) * boxRotationInverse;

	Vec2 closestPosOnBox;
	const auto isCenterInsideBox = circlePosInBoxSpace.x > -boxHalfSize.x && circlePosInBoxSpace.x < boxHalfSize.x
		&& circlePosInBoxSpace.y > -boxHalfSize.y && circlePosInBoxSpace.y < boxHalfSize.y;
	if (isCenterInsideBox) {

		auto distance = std::numeric_limits<float>::infinity();

		if (const auto d = circlePosInBoxSpace.x - -boxHalfSize.x;  d < distance) {
			distance = d;
			closestPosOnBox = Vec2{ -boxHalfSize.x, circlePosInBoxSpace.y };
		}
		if (const auto d = boxHalfSize.x - circlePosInBoxSpace.x;  d < distance) {
			distance = d;
			closestPosOnBox = Vec2{ boxHalfSize.x, circlePosInBoxSpace.y };
		}

		if (const auto d = circlePosInBoxSpace.y - -boxHalfSize.y;  d < distance) {
			distance = d;
			closestPosOnBox = Vec2{ circlePosInBoxSpace.x, -boxHalfSize.y };
		}
		if (const auto d = boxHalfSize.y - circlePosInBoxSpace.y;  d < distance) {
			distance = d;
			closestPosOnBox = Vec2{ circlePosInBoxSpace.x, boxHalfSize.y };
		}
		// Could put this in a loop over axes, but doing the last line of the ifs would be weird and require dot product's or conditionals.

	} else {
		closestPosOnBox = Vec2{
			std::clamp(circlePosInBoxSpace.x, -boxHalfSize.x, boxHalfSize.x),
			std::clamp(circlePosInBoxSpace.y, -boxHalfSize.y, boxHalfSize.y)
		};

		if ((closestPosOnBox - circlePosInBoxSpace).lengthSq() > pow(circle.radius, 2.0f)) {
			return std::nullopt;
		}
	}

	closestPosOnBox = closestPosOnBox * boxRotationInverse.orthonormalInv() + boxPos;
	Vec2 normal = circlePos - closestPosOnBox;
	if (isCenterInsideBox) normal = -normal;

	auto& p = collision.contacts[0];
	collision.contactCount = 1;
	p.normal = normal;
	p.penetrationDepth = p.normal.length() - circle.radius;
	if (isCenterInsideBox) p.penetrationDepth = -(p.normal.length() + circle.radius);
	p.normal = p.normal.normalized();
	p.position = closestPosOnBox;
	p.feature.value = 0;

	return collision;
}

auto collide(Vec2 aPos, float, const CircleCollider& a, Vec2 bPos, float, const CircleCollider& b) -> std::optional<Collision> {
	const auto normal = bPos - aPos;
	const auto distanceSquared = normal.lengthSq();
	if (distanceSquared > pow(a.radius + b.radius, 2.0f)) {
		return std::nullopt;
	}

	Collision collision;
	auto& p = collision.contacts[0];
	collision.contactCount = 1;
	const auto distance = sqrt(distanceSquared);
	p.normal = normal / distance;
	p.penetrationDepth = -(a.radius + b.radius - distance);
	p.normal = (distanceSquared == 0.0f) ? Vec2{ 1.0f, 0.0f } : p.normal.normalized();
	p.position = aPos + p.normal * a.radius;
	p.feature.value = 0;
	return collision;
}

auto contains(Vec2 point, Vec2 pos, float orientation, const BoxCollider& box) -> bool {
	const auto basis = Mat2::rotate(orientation);
	const auto halfSize = box.size / 2.0f;
	const auto p = point - pos;

	const auto
		alongY = dot(p, basis.y()),
		alongX = dot(p, basis.x());

	if (alongX > -halfSize.x && alongX < halfSize.x
		&& alongY > -halfSize.y && alongY < halfSize.y) {
		return true;
	}
	return false;
}

auto contains(Vec2 point, Vec2 pos, float, const CircleCollider& circle) -> bool {
	return (point - pos).lengthSq() <= circle.radius;
}

// What is a good way to destroy an enity if it was hit? Could store a data void* inside each body that would point to the entity.
// TODO: Could pass a class with a callback interface that would determine whether a body should be ignored or not. This is faster than just returning and doing the raycast again from the previous hit ignoring the body that was just hit.
// To ignore a list of bodies is storing a std::set a good option?
auto raycast(Vec2 rayBegin, Vec2 rayEnd, const Collider& collider, Vec2 pos, float orientation) -> std::optional<RaycastResult> {
	return std::visit(
		[&rayBegin, &rayEnd, &pos, &orientation](auto&& collider) -> std::optional<RaycastResult> {
			return raycast(rayBegin, rayEnd, collider, pos, orientation);
		},
		collider
	);
}

auto raycast(Vec2 rayBegin, Vec2 rayEnd, const BoxCollider& collider, Vec2 pos, float orientation) -> std::optional<RaycastResult> {
	// @Performance: is it better to rotate the positions or dot them with the normals. Is this equivalent?
	const auto halfSize = collider.size / 2.0f;
	const auto rotation = Mat2::rotate(orientation);
	const auto inverseRotation = rotation.orthonormalInv();
	const auto start = (rayBegin - pos) * inverseRotation;
	const auto dir = (rayEnd - rayBegin) * inverseRotation;

	Vec2 tEntry, tExit;
	for (usize axis = 0; axis < 2; axis++) {
		if (dir[axis] < 0.0f) {
			// negative / negative = positive
			tEntry[axis] = (halfSize[axis] - start[axis]) / dir[axis];
			tExit[axis] = (-halfSize[axis] - start[axis]) / dir[axis];
		}
		else {
			tEntry[axis] = (-halfSize[axis] - start[axis]) / dir[axis];
			tExit[axis] = (halfSize[axis] - start[axis]) / dir[axis];
		}
	}

	if ((tEntry.x <= 0 && tEntry.y <= 0) || (tExit.x <= 0 && tExit.y <= 0))
		return std::nullopt;

	// Max of entry is the t when it hits both axis and min of exit is when it leaves one of the axis.
	const auto entryT = std::max(tEntry.x, tEntry.y), exitT = std::min(tExit.x, tExit.y);

	if (entryT > exitT)
		return std::nullopt;

	if (entryT > 1.0f)
		return std::nullopt;

	return RaycastResult{
		.t = entryT,
		.normal = (tEntry.x > tEntry.y
			? Vec2{ 1.0f * sign(dir.x), 0.0f }
			: Vec2{ 0.0f, 1.0f * sign(dir.y) }) * rotation,
	};
}

auto raycast(Vec2 rayBegin, Vec2 rayEnd, const CircleCollider& collider, Vec2 pos, float) -> std::optional<RaycastResult> {
	const auto start = rayBegin - pos;
	const auto dir = rayEnd - rayBegin;
	const auto 
		a = dot(dir, dir),
		b = dot(start, dir) * 2.0f,
		c = dot(start, start) - collider.radius * collider.radius;
	const auto discriminant = b * b - 4.0f * a * c;
	if (discriminant < 0.0f)
		return std::nullopt;

	const auto sqrtDiscriminant = sqrt(discriminant);
	const auto 
		t0 = (-b + sqrtDiscriminant) / a / 2.0f,
		t1 = (-b - sqrtDiscriminant) / a / 2.0f;

	const auto t = t0 < t1 ? t0 : t1;

	if (t > 1.0f || t < 0.0f)
		return std::nullopt;

	const auto hitPoint = rayBegin + dir * t;
	return RaycastResult{
		.t = t,
		.normal = (hitPoint - pos).normalized()
	};
}

// An aabb contains a shape if it contains it's aabb, because if an aabb contains a shape <=> it contains the points on it most in the -x, x, -y and y directions (which just means the aabb of the shape) and vice-versa.
auto aabbContains(const Aabb& aabb, const Collider& collider, Vec2 pos, float orientation) -> bool {
	return std::visit(
		[&](auto&& collider) -> bool {
			return aabb.contains(collider.aabb(pos, orientation));
		},
		collider
	);
}