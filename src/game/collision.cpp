#include <game/collision.hpp>
#include <game/game.hpp>
#include <math/mat2.hpp>
#include <math/utils.hpp>
#include <algorithm>
#include <math/transform.hpp>

// When a collision between 2 bodies happens and a collision between the same bodies happened on last frame too, the frame accumulators of contacts with the same features are transfered over.
auto Collision::update(const Collision& newCollision) -> void {
	if (!Game::reusePreviousFrameContactAccumulators) {
		*this = newCollision;
		for (i8 i = 0; i < contactCount; i++) {
			contacts[i].invNormalEffectiveMass = 0.0f;
			contacts[i].invTangentEffectiveMass = 0.0f;
		}
		return;
	}

	normal = newCollision.normal;

	ContactPoint mergedContacts[2];
	for (i8 i = 0; i < newCollision.contactCount; i++) {
		const auto& newContact = newCollision.contacts[i];
		i8 oldContactSameAsNew = -1;
		for (i8 j = 0; j < contactCount; j++) {
			const auto& oldContact = contacts[j];
			const auto sameFeaturePair = newContact.id.featureOnA == oldContact.id.featureOnA && newContact.id.featureOnAIndex == oldContact.id.featureOnAIndex && newContact.id.featureOnB == oldContact.id.featureOnB && newContact.id.featureOnBIndex == oldContact.id.featureOnBIndex;
			if (sameFeaturePair) {
				oldContactSameAsNew = j;
				break;
			}
		}

		if (oldContactSameAsNew != -1) {
			auto& contact = mergedContacts[i];
			const auto& oldContact = contacts[oldContactSameAsNew];
			contact = newContact;
			contact.accumulatedNormalImpluse = oldContact.accumulatedNormalImpluse;
			contact.accumulatedTangentImpulse = oldContact.accumulatedTangentImpulse;
		} else {
			mergedContacts[i] = newContact;
		}
	}

	for (i32 i = 0; i < newCollision.contactCount; ++i)
		contacts[i] = mergedContacts[i];

	contactCount = newCollision.contactCount;
}

auto Collision::preStep(Body& a, Body& b, float invDeltaTime) -> void {
	for (i8 i = 0; i < contactCount; i++) {
		auto& c = contacts[i];
		
		// n = normal
		// cA = contactPosOnA
		// cB = contactPosOnB = contactPosOnA + n
		// C = dot(cA - cB, n)
		// C = dot(pA + rA - (pB + rB), n)
		// C' = dot(velA - velB, n)
		// C' = dot((vA + aA * rA) - (vB + aB * rB), n)
		// JV + b = 0

		//Vec2 r1 = c.pos - a.transform.pos;
		//Vec2 r2 = c.pos - b.transform.pos;

		/*Vec2 r1 = (c.pos + normal * c.separation) - a.pos;
		Vec2 r2 = c.pos - b.pos;*/
		Vec2 r1 = (c.pos + normal * c.separation) - a.transform.pos;
		Vec2 r2 = c.pos - b.transform.pos;

		// Precompute normal mass, tangent mass, and bias.
		float rn1 = dot(r1, normal);
		float rn2 = dot(r2, normal);
		float kNormal = a.invMass + b.invMass;
		kNormal += a.invRotationalInertia * (dot(r1, r1) - rn1 * rn1) + b.invRotationalInertia * (dot(r2, r2) - rn2 * rn2);
		c.invNormalEffectiveMass = 1.0f / kNormal;

		Vec2 tangent = cross(normal, 1.0f);
		float rt1 = dot(r1, tangent);
		float rt2 = dot(r2, tangent);
		float kTangent = a.invMass + b.invMass;
		kTangent += a.invRotationalInertia * (dot(r1, r1) - rt1 * rt1) + b.invRotationalInertia * (dot(r2, r2) - rt2 * rt2);
		c.invTangentEffectiveMass = 1.0f / kTangent;

		// Baumgarte stabilization bias.
		// The velocity needed to solve the penetration in a single frame is when bias = 1.
		// bias = factor * (penetration / deltaTime)
		// bias = [scalar] * [velocity]
		const auto biasFactor = 0.2f;
		// Decreasing the allowed penetration makes things more bouncy.
		/*const auto k_allowedPenetration = 0.01f;*/
		// Some penetration is allowed to decrease jitter, especially when multiple objects are stacked on top of eachother. This is sometimes called slop.
		const auto ALLOWED_PENETRATION = 0.01f;

		auto relativeVelAtContact = (b.vel + cross(b.angularVel, r2)) - (a.vel + cross(a.angularVel, r1));

		/*c.bias = -biasFactor * invDeltaTime * std::min(0.0f, c.separation + ALLOWED_PENETRATION);*/
		c.bias = -biasFactor * invDeltaTime * std::min(0.0f, -std::abs(c.separation) + ALLOWED_PENETRATION);
		/*c->bias = std::max(c->bias, -dot(b->vel - a->vel, c->normal)) * 0.5f;*/
		/*c->bias = std::max(c->bias, -dot(relativeVelAtContact, c->normal) * 0.2f);
		c->bias = std::max(0.0f, c->bias);*/


		if (Game::accumulateImpulses) {
			// Apply normal + friction impulse
			Vec2 P = c.accumulatedNormalImpluse * normal + c.accumulatedTangentImpulse * tangent;

			a.vel -= a.invMass * P;
			a.angularVel -= a.invRotationalInertia * cross(r1, P);

			b.vel += b.invMass * P;
			b.angularVel += b.invRotationalInertia * cross(r2, P);
		}
	}
}

auto Collision::applyImpulse(Body& a, Body& b) -> void {
	for (i32 i = 0; i < contactCount; i++) {
		auto& contact = contacts[i];
		//ASSERT(contact.separation <= 0.0f);

		/*const auto r1 = contact.pos - a.transform.pos;
		const auto r2 = contact.pos - b.transform.pos;*/
		Vec2 r1 = (contact.pos + normal * contact.separation) - a.transform.pos;
		Vec2 r2 = contact.pos - b.transform.pos;
		auto relativeVelAtContact = (b.vel + cross(b.angularVel, r2)) - (a.vel + cross(a.angularVel, r1));

		// Compute normal impulse
		// You apply the bias if the objects are moving slowly (relative velocity is small) otherwise this bias is the restitution if they are moving fast (large relative velocity). Also, make sure you only calculate this bias once for every time step, and  not for every iteration of impulses.
		const auto velocityAlongNormal = dot(relativeVelAtContact, normal);

		auto normalImpulse = contact.invNormalEffectiveMass * (-velocityAlongNormal + contact.bias);

		if (Game::accumulateImpulses) {
			const auto oldNormalImpluse = contact.accumulatedNormalImpluse;
			contact.accumulatedNormalImpluse = std::max(oldNormalImpluse + normalImpulse, 0.0f);
			normalImpulse = contact.accumulatedNormalImpluse - oldNormalImpluse;
		} else {
			normalImpulse = std::max(normalImpulse, 0.0f);
		}

		const auto impulseNormal = normalImpulse * normal;

		a.vel -= a.invMass * impulseNormal;
		a.angularVel -= a.invRotationalInertia * cross(r1, impulseNormal);

		b.vel += b.invMass * impulseNormal;
		b.angularVel += b.invRotationalInertia * cross(r2, impulseNormal);

		relativeVelAtContact = b.vel + cross(b.angularVel, r2) - a.vel - cross(a.angularVel, r1);

		Vec2 tangent = cross(normal, 1.0f);
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

auto BoxCollider::aabb(const Transform& transform) const -> Aabb {
	const auto corners = getCorners(transform.pos, transform.angle());
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

auto CircleCollider::aabb(const Transform& transform) const -> Aabb {
	return Aabb{ transform.pos + Vec2{ -radius }, transform.pos + Vec2{ radius } };
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

auto aabb(const Collider& collider, const Transform& transform) -> Aabb {
	return std::visit(
		[&transform](auto&& collider) -> Aabb {
			return collider.aabb(transform);
		},
		collider
	);
}

auto collide(const Transform& aTransform, const Collider& aCollider, const Transform& bTransform, const Collider& bCollider) -> std::optional<Collision>
{
#define GET(collider, name, type) const auto name = std::get_if<type>(&collider); name != nullptr

	if (GET(aCollider, aBox, BoxCollider)) {
		if (GET(bCollider, bBox, BoxCollider)) {

			return collide(aTransform, *aBox, bTransform, *bBox);
			/*return collide(aPos, aOrientation, *aBox, bPos, bOrientation, *bBox);*/
		} else if (GET(bCollider, bCircle, CircleCollider)) {
			return collide(aTransform, *aBox, bTransform, *bCircle);
		}
	} else if (GET(aCollider, aCircle, CircleCollider)) {
		if (GET(bCollider, bCircle, CircleCollider)) {
			return collide(aTransform, *aCircle, bTransform, *bCircle);
		}
		if (GET(bCollider, bBox, BoxCollider)) {
			auto collision = collide(bTransform, *bBox, aTransform, *aCircle);
			if (collision.has_value()) {
				collision->normal = -collision->normal;
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

struct Separation {
	float separation;
	i16 edgeIndex;
};

// Performs SAT on the shapes a and b. To separate shapes translate shape b by a.normals[edgeIndex] * separation. Returns nullopt if the shapes are already separating.
static auto minSeparation(const std::vector<Vec2>& aVerts, const std::vector<Vec2>& aNormals, const Transform& aTransform, std::vector<Vec2> bVerts, const Transform& bTransform) -> std::optional<Separation> {
	// Finds the axis of least separation. The least distance the shapes need to be translated to stop overlapping. For example when you put the a box on to of a box then their shadows fully overlap so the least separation, not the max separation axis / face normal is desired.
	auto minSeparation = std::numeric_limits<float>::infinity();
	i16 minSeparationNormalIndex = 0;

	for (i16 normalIndex = 0; normalIndex < aNormals.size(); normalIndex++) {
		const auto n = aNormals[normalIndex];

		// min and max offsets of vertices of a and b along the normal n (the projections onto the normal). 
		auto minA = std::numeric_limits<float>::infinity();
		auto maxA = -std::numeric_limits<float>::infinity();
		auto minB = minA;
		auto maxB = maxA;

		for (auto v : aVerts) {
			const auto d = dot(n, v);
			if (d < minA) minA = d;
			if (d > maxA) maxA = d;
		}

		const auto bToAObjectSpace = bTransform * aTransform.inversed();
		for (auto v : bVerts) {
			// Convert only B into A's object space instead of converting everything into world space to reduce calculations. 
			v *= bToAObjectSpace;
			const auto d = dot(n, v);
			if (d < minB) minB = d;
			if (d > maxB) maxB = d;
		}

		if (const auto projectionsOverlap = minA <= maxB && maxA >= minB) {
			auto separation = maxA - minB;
			if (separation < minSeparation) {
				minSeparation = separation;
				minSeparationNormalIndex = normalIndex;
			}
		} else {
			// If there is any axis on which the projections don't overlap the shapes don't overlap. To optimize could store this axis and check it first the next frame.
			return std::nullopt;
		}
	}

	return Separation{
		minSeparation,
		minSeparationNormalIndex
	};
}

struct Contact {
	Vec2 pos;
	float penetration;
};

// The computed contact manifold tries to approximate the contact points at which the collision started. These points are only an approximation.
// This code often uses face indices as vertex indices, this is explained in ConvexPolygon.
auto collide(const ConvexPolygon& a, const Transform& aTransform, const ConvexPolygon& b, const Transform& bTransform) -> std::optional<Collision> {
	// minSeparation() only checks the normals of the first arguments so both shapes have to be tested.
	const auto aSeparation = minSeparation(a.verts, a.normals, aTransform, b.verts, bTransform);
	if (!aSeparation.has_value())
		return std::nullopt;

	const auto bSeparation = minSeparation(b.verts, b.normals, bTransform, a.verts, aTransform);
	if (!bSeparation.has_value())
		return std::nullopt;

	const ConvexPolygon* reference;
	const Transform* referenceTransform;
	const ConvexPolygon* incident;
	const Transform* incidentTransform;
	i16 referenceFaceIndex;

	if (aSeparation->separation < bSeparation->separation) {
		reference = &a;
		incident = &b;
		referenceTransform = &aTransform;
		incidentTransform = &bTransform;
		referenceFaceIndex = aSeparation->edgeIndex;
	} else {
		reference = &b;
		incident = &a;
		referenceTransform = &bTransform;
		incidentTransform = &aTransform;
		referenceFaceIndex = bSeparation->edgeIndex;
	}

	const auto normal = reference->normals[referenceFaceIndex] * referenceTransform->rot;
	i16 furthestVertOfIndidentInsideReference = 0;
	auto maxDistance = std::numeric_limits<float>::infinity();
	for (i16 i = 0; i < incident->verts.size(); i++) {
		const auto d = dot(normal, incident->verts[i] * *incidentTransform);
		// Using less than because the normal points towards the shape so the get the furthest point inside you need to find the smallest value of the projection.
		if (d < maxDistance) {
			maxDistance = d;
			furthestVertOfIndidentInsideReference = i;
		}
	}

	const auto face0Index = furthestVertOfIndidentInsideReference;
	const auto face1Index = static_cast<i16>(furthestVertOfIndidentInsideReference - 1 < 0
		? static_cast<i16>(incident->normals.size()) - 1
		: furthestVertOfIndidentInsideReference - 1);
	const auto face0Normal = incident->normals[face0Index] * incidentTransform->rot;
	const auto face1Normal = incident->normals[face1Index] * incidentTransform->rot;

	i16 incidentFace;
	// Choose incident face to be the face that is the most parallel to the reference face.
	if (dot(normal, face0Normal) < dot(normal, face1Normal)) {
		incidentFace = face0Index;
	} else {
		incidentFace = face1Index;
	}

	struct ClipVert {
		ContactPointId id;
		Vec2 pos;
	};

	const auto incidentFirstVertIndex = incidentFace;
	const auto incidentSecondVertIndex = static_cast<i16>(incidentFace + 1 >= static_cast<i16>(incident->verts.size()) ? 0 : incidentFace + 1);
	ClipVert vertsToClip[2]{
		{
			.id = {
				.featureOnA = ContactPointFeature::FACE, .featureOnAIndex = referenceFaceIndex,
				.featureOnB = ContactPointFeature::VERTEX, .featureOnBIndex = incidentFirstVertIndex
			},
			.pos = incident->verts[incidentFirstVertIndex] * *incidentTransform
		},
		{
			.id = {
				.featureOnA = ContactPointFeature::FACE, .featureOnAIndex = referenceFaceIndex,
				.featureOnB = ContactPointFeature::VERTEX, .featureOnBIndex = incidentSecondVertIndex
			},
			.pos = incident->verts[incidentSecondVertIndex] * *incidentTransform
		}
	};

	auto clipPoints = [&](ClipVert verts[2], const Line& line, i16 aVertIndex) -> void {
		auto clipPoint = [&](ClipVert vert, const Line& line, const Line& edgeLine, i16 aVertIndex) -> ClipVert {
			auto distanceFromLine = signedDistance(line, vert.pos);
			if (distanceFromLine > 0.0f) {
				const auto intersection = line.intersection(edgeLine);
				if (intersection.has_value()) {
					vert.pos = *intersection;
					vert.id.featureOnA = ContactPointFeature::VERTEX;
					vert.id.featureOnAIndex = aVertIndex;
					vert.id.featureOnB = ContactPointFeature::FACE;
					vert.id.featureOnBIndex = incidentFace;
				}
			}
			return vert;
		};

		Line edgeLine{ verts[0].pos, verts[1].pos };

		verts[0] = clipPoint(verts[0], line, edgeLine, aVertIndex);
		verts[1] = clipPoint(verts[1], line, edgeLine, aVertIndex);
	};

	const auto referenceFaceEndPoint0 = reference->verts[referenceFaceIndex] * *referenceTransform;
	const auto referenceFaceEndPoint1 = reference->verts[referenceFaceIndex + 1 >= static_cast<i16>(reference->verts.size()) ? 0 : referenceFaceIndex + 1] * *referenceTransform;

	// The normals are flipped to make the positive half space of the line inside the reference face segment.
	Line lineA{ referenceFaceEndPoint0, referenceFaceEndPoint0 - normal };
	clipPoints(vertsToClip, lineA, referenceFaceIndex);
	Line lineB{ referenceFaceEndPoint1, referenceFaceEndPoint1 + normal };
	clipPoints(vertsToClip, lineB, referenceFaceIndex + 1 >= static_cast<i16>(reference->verts.size()) ? 0 : referenceFaceIndex + 1);

	Collision manifold;
	manifold.normal = normal;
	manifold.contactCount = 0;
	std::vector<Vec2> points;
	Line referenceFaceLine{ referenceFaceEndPoint0, referenceFaceEndPoint1 };

	// Remove all the contacts that lie outside the reference shape. On the negative half space of the reference face.
	const auto distanceFromReferenceFace0 = signedDistance(referenceFaceLine, vertsToClip[0].pos);
	if (distanceFromReferenceFace0 >= 0.0f) {
		manifold.contacts[manifold.contactCount] = ContactPoint{
			.pos = vertsToClip[0].pos,
			.separation = distanceFromReferenceFace0,
			.id = vertsToClip[0].id,
		};
		manifold.contactCount++;
	}
	const auto distanceFromReferenceFace1 = signedDistance(referenceFaceLine, vertsToClip[1].pos);
	if (distanceFromReferenceFace1 >= 0.0f) {
		manifold.contacts[manifold.contactCount] = ContactPoint{
			.pos = vertsToClip[1].pos,
			.separation = distanceFromReferenceFace1,
			.id = vertsToClip[1].id,
		};
		manifold.contactCount++;
	}

	// All the previous code assuemes that a is the reference shape. If that is not true the features have to be swapped.
	if (const auto swap = reference == &b) {
		manifold.normal = -manifold.normal;
		for (i32 i = 0; i < manifold.contactCount; i++) {
			auto& point = manifold.contacts[i];

			point.pos = point.pos - manifold.normal * point.separation;
			//point.separation = -point.separation;
			std::swap(point.id.featureOnA, point.id.featureOnB);
			std::swap(point.id.featureOnAIndex, point.id.featureOnBIndex);
		}
	}

	/*for (i32 i = 0; i < manifold.contactCount; i++) {
		auto& point = manifold.contacts[i];
		point.separation = -std::abs(point.separation);
	}*/

	return manifold;
}

auto collide(const Transform& aTransform, const BoxCollider& aBox, const Transform& bTransform, const BoxCollider& bBox) -> std::optional<Collision> {
	static ConvexPolygon aShape{ .verts = std::vector<Vec2>(4, Vec2{ 0.0f }), .normals = std::vector<Vec2>(4, Vec2{ 0.0f }) };
	aShape.verts[3] = aBox.size / 2.0f;
	aShape.verts[2] = Vec2{ aBox.size.x, -aBox.size.y } / 2.0f;
	aShape.verts[1] = -aBox.size / 2.0f;
	aShape.verts[0] = Vec2{ -aBox.size.x, aBox.size.y } / 2.0f;
	static ConvexPolygon bShape{ .verts = std::vector<Vec2>(4, Vec2{ 0.0f }), .normals = std::vector<Vec2>(4, Vec2{ 0.0f }) };
	bShape.verts[3] = bBox.size / 2.0f;
	bShape.verts[2] = Vec2{ bBox.size.x, -bBox.size.y } / 2.0f;
	bShape.verts[1] = -bBox.size / 2.0f;
	bShape.verts[0] = Vec2{ -bBox.size.x, bBox.size.y } / 2.0f;

	aShape.calculateNormals();
	bShape.calculateNormals();

	return collide(aShape, aTransform, bShape, bTransform);
}

// Another way to do this that would work is to do the same thing if the center is inside the box else calculate the seperation vector by calculating the distance from the sides of the box to the center. The length would then be circle.radius - v.length().
// Hacky but working code.
//Vec2 distanceFromBoxSideToCircleCenter = {
//	std::max(0.0f, abs(alongX) - boxHalfSize.x) * -sign(alongX),
//	std::max(0.0f, abs(alongY) - boxHalfSize.y) * -sign(alongY)
//};
auto collide(const Transform& aTransform, const BoxCollider& box, const Transform& bTransform, const CircleCollider& circle) -> std::optional<Collision> {
	auto boxOrientation = aTransform.angle();
	auto boxPos = aTransform.pos;
	auto circlePos = bTransform.pos;
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
	collision.normal = normal;
	p.separation = collision.normal.length() - circle.radius;
	if (isCenterInsideBox) p.separation = -(collision.normal.length() + circle.radius);
	collision.normal = collision.normal.normalized();
	p.pos = closestPosOnBox;
	p.id = ContactPointId{ .featureOnA = ContactPointFeature::FACE, .featureOnAIndex = 0, .featureOnB = ContactPointFeature::FACE, .featureOnBIndex = 0 };

	return collision;
}

auto collide(const Transform& aTransform, const CircleCollider& a, const Transform& bTransform, const CircleCollider& b) -> std::optional<Collision> {
	const auto aPos = aTransform.pos;
	const auto bPos = bTransform.pos;
	const auto normal = bPos - aPos;
	const auto distanceSquared = normal.lengthSq();
	if (distanceSquared > pow(a.radius + b.radius, 2.0f)) {
		return std::nullopt;
	}

	Collision collision;
	auto& p = collision.contacts[0];
	collision.contactCount = 1;
	const auto distance = sqrt(distanceSquared);
	collision.normal = normal / distance;
	p.separation = -(a.radius + b.radius - distance);
	collision.normal = (distanceSquared == 0.0f) ? Vec2{ 1.0f, 0.0f } : collision.normal.normalized();
	p.pos = aPos + collision.normal * a.radius;
	p.id = ContactPointId{ .featureOnA = ContactPointFeature::FACE, .featureOnAIndex = 0, .featureOnB = ContactPointFeature::FACE, .featureOnBIndex = 0 };
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
auto raycast(Vec2 rayBegin, Vec2 rayEnd, const Collider& collider, const Transform& transform) -> std::optional<RaycastResult> {
	return std::visit(
		[&rayBegin, &rayEnd, &transform](auto&& collider) -> std::optional<RaycastResult> {
			return raycast(rayBegin, rayEnd, collider, transform);
		},
		collider
	);
}

auto raycast(Vec2 rayBegin, Vec2 rayEnd, const BoxCollider& collider, const Transform& transform) -> std::optional<RaycastResult> {
	const auto pos = transform.pos;
	const auto orientation = transform.angle();
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

auto raycast(Vec2 rayBegin, Vec2 rayEnd, const CircleCollider& collider, const Transform& transform) -> std::optional<RaycastResult> {
	const auto pos = transform.pos;
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
			return aabb.contains(collider.aabb(Transform{ pos, orientation }));
		},
		collider
	);
}