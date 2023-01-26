#include <demos/testDemo.hpp>
#include <game/collision/collision.hpp>
#include <engine/debug.hpp>
#include <engine/input.hpp>
#include <math/mat2.hpp>
#include <engine/renderer.hpp>
#include <math/transform.hpp>
#include <engine/frameAllocator.hpp>
#include <string>

struct ConvexPolygon {
	// Could store the verts and normal into a vector of structs with edgeBegin and edgeNormal, but that would probably be confusing.

	std::vector<Vec2> verts;
	// The normal i belong to the face with endpoints verts[i] and verts[(i + 1) % size].
	std::vector<Vec2> normals;

	auto calculateNormals() -> void {
		normals.clear();
		if (verts.size() < 3) {
			ASSERT_NOT_REACHED();
			return;
		}

		for (usize i = 0; i < verts.size() - 1; i++) {
			normals.push_back((verts[i + 1] - verts[i]).rotBy90deg().normalized());
		}
		normals.push_back((verts[0] - verts.back()).rotBy90deg().normalized());
	}
};

enum class ContactPointFeature : u8 {
	EDGE, VERTEX
};

struct ContactPointId {
	ContactPointFeature featureOnA;
	u16 featureOnAIndex;
	ContactPointFeature featureOnB;
	u16 featureOnBIndex;
};

struct ContactManifoldPoint {
	Vec2 pos;
	float penetration;
	ContactPointId id;
};

struct ContactManifold {
	// SAT with clipping can return at most 2 contactPoints. I don't think there is a case when a convex shape would need more than 2 contact points. There is either face vs face, face vs vertex or vertex vs vertex.
	ContactManifoldPoint contactPoints[2];
	i32 contactPointsCount;
	Vec2 normal;

	float invNormalEffectiveMass;
	float invTangentEffectiveMass;

	float accumulatedNormalImpluse = 0.0f;
	float accumulatedTangentImpulse = 0.0f;
};

#include <math/utils.hpp>

static auto makeRegularPolygon(i32 sides, float radius) -> ConvexPolygon {
	ConvexPolygon polygon;
	const auto angleStep = TAU<float> / sides;
	for (i32 i = 0; i < sides; i++) {
		polygon.verts.push_back(Vec2::oriented(angleStep / 2.0f + angleStep * i) * radius);
	}
	polygon.calculateNormals();
	return polygon;
};

struct Separation {
	float separation;
	i32 edgeIndex;
};

// Performs SAT on the shapes a and b. To separate shapes translate shape b by a.normals[edgeIndex] * separation. Returns nullopt if the shapes are already separating.
static auto minSeparation(const std::vector<Vec2>& aVerts, const std::vector<Vec2>& aNormals, const Transform& aTransform, std::vector<Vec2> bVerts, const Transform& bTransform) -> std::optional<Separation> {
	// Finds the axis of least separation. The least distance the shapes need to be translated to stop overlapping. For example when you put the a box on to of a box then their shadows fully overlap so the least separation, not the max separation axis / face normal is desired.
	auto minSeparation = std::numeric_limits<float>::infinity();
	i32 minSeparationNormalIndex = 0;

	for (i32 nI = 0; nI < aNormals.size(); nI++) {
		const auto n = aNormals[nI];

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
				minSeparationNormalIndex = nI;
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

static auto collide(const ConvexPolygon& a, const Transform& aTransform, const ConvexPolygon& b, const Transform& bTransform) -> std::optional< ContactManifold>{
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
	Vec2 normal;

	Vec2 edgeStart;
	Vec2 edgeEnd;
	u32 referenceEdge;

	Vec2 referenceFaceMidPoint;

	float s;
	auto flip = false;
	if (aSeparation->separation < bSeparation->separation) {
		reference = &a;
		incident = &b;
		normal = a.normals[aSeparation->edgeIndex] * aTransform.rot * aSeparation->separation;
		s = aSeparation->separation;
		referenceFaceMidPoint = (a.verts[aSeparation->edgeIndex] + a.verts[(aSeparation->edgeIndex + 1) % a.verts.size()]) / 2.0f * aTransform;
		referenceTransform = &aTransform;
		incidentTransform = &bTransform;
		edgeStart = a.verts[aSeparation->edgeIndex] * *referenceTransform;
		edgeEnd = a.verts[(aSeparation->edgeIndex + 1) % a.verts.size()] * *referenceTransform;
		referenceEdge = aSeparation->edgeIndex;
	} else {
		flip = true;
		reference = &b;
		incident = &a;
		normal = b.normals[bSeparation->edgeIndex] * bTransform.rot * bSeparation->separation;
		referenceFaceMidPoint = (b.verts[bSeparation->edgeIndex] + b.verts[(bSeparation->edgeIndex + 1) % b.verts.size()]) / 2.0f * bTransform;
		referenceTransform = &bTransform;
		incidentTransform = &aTransform;
		edgeStart = b.verts[bSeparation->edgeIndex] * *referenceTransform;
		edgeEnd = b.verts[(bSeparation->edgeIndex + 1) % b.verts.size()] * *referenceTransform;
		s = bSeparation->separation;
		referenceEdge = bSeparation->edgeIndex;
	}

	//Debug::drawRay(referenceFaceMidPoint, normal, Vec3::RED);
	//Debug::drawRay(referenceFaceMidPoint, normal / s);

	i32 furthestVertOfIndidentInsideReference = 0;
	auto maxDistance = std::numeric_limits<float>::infinity();
	for (usize i = 0; i < incident->verts.size(); i++) {
		auto d = dot(normal, incident->verts[i] * *incidentTransform);
		// Using less than because the normal points towards the shape so the get the furthest point inside you need to find the smallest value of the projection.
		if (d < maxDistance) {
			maxDistance = d;
			furthestVertOfIndidentInsideReference = i;
		}
	}

	auto getEdges = [](const ConvexPolygon* p, u32 i, const Transform& transform) -> std::pair<Vec2, Vec2> {
		return { p->verts[i] * transform, p->verts[(i + 1) % p->verts.size()] * transform };
	};

	auto drawEdge = [](const ConvexPolygon* p, u32 i, const Transform& transform) -> void {
		Debug::drawLine(p->verts[i] * transform, p->verts[(i + 1) % p->verts.size()] * transform, Vec3::RED);
	};


	auto face0Index = furthestVertOfIndidentInsideReference;
	auto face1Index = (furthestVertOfIndidentInsideReference - 1);
	if (face1Index < 0) {
		face1Index = incident->normals.size() - 1;
	}
	auto face0 = incident->normals[face0Index] * incidentTransform->rot;
	auto face1 = incident->normals[face1Index] * incidentTransform->rot;

	i32 incidentFace;
	if (dot(normal, face0) < dot(normal, face1)) {
		incidentFace = face0Index;
	} else {
		incidentFace = face1Index;
	}
	//Debug::drawLine(edgeStart, edgeEnd, Vec3::GREEN);
	//drawEdge(incident, incidentFace, *incidentTransform);

	struct ClipVert {
		ContactPointId id;
		Vec2 pos;
	};

	const auto incidentFirstVertIndex = incidentFace;
	const auto incidentSecondVertIndex = incidentFace + 1 >= incident->verts.size() ? 0 : incidentFace + 1;
	ClipVert incidentFaceEndpoints[2]{
		{
			.id = { 
				.featureOnA = ContactPointFeature::EDGE, .featureOnAIndex = static_cast<u16>(referenceEdge),
				.featureOnB = ContactPointFeature::VERTEX, .featureOnBIndex = static_cast<u16>(incidentFirstVertIndex)
			},
			.pos = incident->verts[incidentFirstVertIndex] * *incidentTransform
		},
		{
			.id = {
				.featureOnA = ContactPointFeature::EDGE, .featureOnAIndex = static_cast<u16>(referenceEdge),
				.featureOnB = ContactPointFeature::VERTEX, .featureOnBIndex = static_cast<u16>(incidentSecondVertIndex)
			},
			.pos = incident->verts[incidentSecondVertIndex] * *incidentTransform
		}
	};
	/*Debug::drawPoint(incidentFaceEndpoints[0].pos, Vec3::RED);
	Debug::drawPoint(incidentFaceEndpoints[1].pos, Vec3::RED);*/

	auto clipPoints = [&](ClipVert verts[2], const Line& line, i16 aVertIndex) -> void {
		auto clipPoint = [&](ClipVert vert, const Line& line, const Line& edgeLine, i16 aVertIndex) -> ClipVert {
			auto distanceFromLine = signedDistance(line, vert.pos);
			if (distanceFromLine > 0.0f) {
				const auto intersection = line.intersection(edgeLine);
				if (intersection.has_value()) {
					vert.pos = *intersection;
					vert.id.featureOnA = ContactPointFeature::VERTEX;
					vert.id.featureOnAIndex = aVertIndex;
					vert.id.featureOnB = ContactPointFeature::EDGE;
					/*vert.id.featureOnBIndex = vert.id.featureOnBIndex;*/
					vert.id.featureOnBIndex = incidentFace;
				}
			}
			return vert;
		};

		Line edgeLine{ verts[0].pos, verts[1].pos };

		verts[0] = clipPoint(verts[0], line, edgeLine, aVertIndex);
		verts[1] = clipPoint(verts[1], line, edgeLine, aVertIndex);
	};

	Line lineA{ edgeStart, edgeStart - normal };
	clipPoints(incidentFaceEndpoints, lineA, referenceEdge);
	Line lineB{ edgeEnd, edgeEnd + normal };
	clipPoints(incidentFaceEndpoints, lineB, referenceEdge + 1 >= reference->verts.size() ? 0 : referenceEdge + 1);


	ContactManifold manifold;
	manifold.normal = normal;
	manifold.contactPointsCount = 0;
	std::vector<Vec2> points;
	Line incidentFaceLine{ edgeStart, edgeEnd };
	auto d0 = signedDistance(incidentFaceLine, incidentFaceEndpoints[0].pos);
	if (d0 >= 0.0f) {
		Debug::drawRay(incidentFaceEndpoints[0].pos, normal * d0);
		manifold.contactPoints[manifold.contactPointsCount] = ContactManifoldPoint{ 
			.pos = incidentFaceEndpoints[0].pos,
			.penetration = d0, 
			.id = incidentFaceEndpoints[0].id, 
		};
		manifold.contactPointsCount++;
	}
	auto d1 = signedDistance(incidentFaceLine, incidentFaceEndpoints[1].pos);
	if (d1 >= 0.0f) {
		Debug::drawRay(incidentFaceEndpoints[1].pos, normal * d1);
		manifold.contactPoints[manifold.contactPointsCount] = ContactManifoldPoint{
			.pos = incidentFaceEndpoints[1].pos,
			.penetration = d1,
			.id = incidentFaceEndpoints[1].id,
		};
		manifold.contactPointsCount++;
	}
	auto flipFeatures = [](ContactManifoldPoint& point) {
		std::swap(point.id.featureOnA, point.id.featureOnB);
		std::swap(point.id.featureOnAIndex, point.id.featureOnBIndex);
	};

	Debug::drawLine(Vec2{ 0.0f }, normal * d0);
	Debug::drawLine(Vec2{ 0.0f }, normal * d1);

	if (flip) {
		for (i32 i = 0; i < manifold.contactPointsCount; i++) {
			flipFeatures(manifold.contactPoints[i]);
		}
	}

	return manifold;

	/*for (auto& p : points) {
		Debug::drawPoint(p, Vec3::RED);
	}*/

	//auto incidentEdges = getEdges(incident, incidentFace, *incidentTransform);

	//Line lineA{ edgeStart, edgeStart - normal };
	//Line lineB{ edgeEnd, edgeEnd + normal };

	//auto clipPoints = [](const std::pair<Vec2, Vec2>& points, const Line& line) -> std::pair<Vec2, Vec2> {
	//	auto clipPoint = [](Vec2 p, const Line& line, const Line& edgeLine) -> Vec2 {
	//		auto distanceFromLine = signedDistance(line, p);
	//		if (distanceFromLine > 0.0f) {
	//			auto x = line.intersection(edgeLine);
	//			if (!x.has_value()) {
	//				Debug::drawPoint(Vec2{ 0.0f }, Vec3::GREEN);
	//			}

	//			return *x;
	//		}
	//		return p;
	//	};

	//	Line edgeLine{ points.first, points.second };

	//	return {
	//		clipPoint(points.first, line, edgeLine),
	//		clipPoint(points.second, line, edgeLine)
	//	};
	//};

	//auto cliped = clipPoints(incidentEdges, lineA);
	//cliped = clipPoints(cliped, lineB);

	//std::vector<Vec2> points;
	//Line incidentFaceLine{ edgeStart, edgeEnd };
	//auto d = signedDistance(incidentFaceLine, cliped.first);
	//if (d >= 0.0f) {
	//	points.push_back(cliped.first);
	//}

	//d = signedDistance(incidentFaceLine, cliped.second);
	//if (d >= 0.0f) {
	//	points.push_back(cliped.second);
	//}


	//for (auto& p : points) {
	//	Debug::drawPoint(p, Vec3::RED);
	//}
}


auto drawPolygon(const ConvexPolygon& polygon, const Transform& transform) {
	const auto& [verts, normals] = polygon;
	for (usize i = 0; i < verts.size() - 1; i++) {
		Debug::drawLine(verts[i] * transform, verts[i + 1] * transform);
	}
	Debug::drawLine(verts.back() * transform, verts[0] * transform);

	for (usize i = 0; i < verts.size(); i++) {
		Debug::drawText(verts[i] * transform, frameAllocator.format("v%zu", i).data(), Vec3::WHITE, 0.03f);
	}

	for (usize i = 0; i < verts.size() - 1; i++) {
		auto midPoint = (verts[i] * transform + verts[i + 1] * transform) / 2.0f;
		Debug::drawText(midPoint, frameAllocator.format("e%zu", i).data(), Vec3::WHITE, 0.03f);
	}
	auto midPoint = (verts.back() * transform + verts[0] * transform) / 2.0f;
	Debug::drawText(midPoint, frameAllocator.format("e%zu", verts.size() - 1).data(), Vec3::WHITE, 0.03f);
}

auto TestDemo::update() -> void {
	Camera camera;

	static float aOrientation = 0.0f;
	if (Input::isKeyHeld(Keycode::A)) {
		aOrientation += 0.01f;
	} else if (Input::isKeyHeld(Keycode::D)) {
		aOrientation -= 0.01f;
	}
	
	Transform aTransform{ camera.cursorPos(), aOrientation };
	auto a = makeRegularPolygon(6, 0.15f);

	Transform bTransform{ Vec2{ 0.3f, 0.2f }, 0.0f };
	auto b = makeRegularPolygon(4, 0.2f);

	drawPolygon(a, aTransform);
	drawPolygon(b, bTransform);
	auto manifold = collide(a, aTransform, b, bTransform);
	if (manifold.has_value()) {
		for (i32 i = 0; i < manifold->contactPointsCount; i++) {
			const auto& point = manifold->contactPoints[i];

			auto x = (point.id.featureOnA == ContactPointFeature::EDGE ? "e" : "v") + std::to_string(point.id.featureOnAIndex)
				+ ((point.id.featureOnB == ContactPointFeature::EDGE) ? "e" : "v") + std::to_string(point.id.featureOnBIndex);
			Debug::drawText(point.pos + Vec2{ 0.0f, 0.05f }, x.data(), Vec3::GREEN, 0.03f);
			Debug::drawPoint(point.pos, Vec3::RED);
			Debug::drawRay(point.pos, manifold->normal.normalized() * point.penetration, Vec3::GREEN);
		}
	}
	

	Renderer::update(camera);
}
