#include <demos/testDemo.hpp>
#include <game/collision/collision.hpp>
#include <engine/debug.hpp>
#include <engine/input.hpp>
#include <math/mat2.hpp>
#include <engine/renderer.hpp>
#include <math/transform.hpp>

//struct Edge {
//	Vec2 edgeStartVert;
//	Vec2 edgeNormal;
//};

struct ConvexPolygon {
	std::vector<Vec2> verts;
	std::vector<Vec2> normals;

	auto calculateNormals() -> void {
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

struct ContactManifoldPoint {
	Vec2 pos;
	float penetration;
	i32 id;
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

struct Separation {
	float separation;
	i32 edgeIndex;
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

static auto minSeparation(const ConvexPolygon& a, Vec2 aPos, const Mat2& aRot, const ConvexPolygon& b, Vec2 bPos, const Mat2& bRot) -> std::optional<Separation> {
	auto maxSeparation = -std::numeric_limits<float>::infinity();
	i32 maxSeparationNormal = 0;

	for (i32 nI = 0; nI < a.normals.size(); nI++) {
		auto n = a.normals[nI];
		n *= aRot;

		auto minA = std::numeric_limits<float>::infinity();
		auto maxA = -minA;
		auto minB = minA, maxB = maxA;

		for (auto v : a.verts) {
			v = v * aRot + aPos;
			const auto d = dot(n, v);
			if (d < minA) {
				minA = d;
			}
			if (d > maxA) {
				maxA = d;
			}
		}

		for (auto v : b.verts) {
			v = v * bRot + bPos;
			const auto d = dot(n, v);
			if (d < minB) {
				minB = d;
			}
			if (d > maxB) {
				maxB = d;
			}
		}

		if (minA <= maxB && maxA >= minB) {
			auto separation = minB - maxA;
			if (separation > maxSeparation) {
				maxSeparation = separation;
				maxSeparationNormal = nI;
			}
		} else {
			auto min = a.normals[nI] * aRot * 0.1f;
			auto edge = (a.verts[nI] + a.verts[(nI + 1) % a.verts.size()]) / 2.0f * aRot * 0.1f + aPos;
			return std::nullopt;
		}
	}

	return Separation{
		maxSeparation,
		maxSeparationNormal
	};
}

//struct Transform {
//
//};

struct Contact {
	Vec2 pos;
	float penetration;
};

static auto collide(const ConvexPolygon& a, Vec2 aPos, float aOrientation, const ConvexPolygon& b, Vec2 bPos, float bOrientation) -> void {
	Vec2 min;
	Vec2 edge;

	auto aS = minSeparation(a, aPos, Mat2::rotate(aOrientation), b, bPos, Mat2::rotate(bOrientation));
	if (!aS.has_value())
		return;
	auto bS = minSeparation(b, bPos, Mat2::rotate(bOrientation), a, aPos, Mat2::rotate(aOrientation));
	if (!bS.has_value())
		return;

	const ConvexPolygon* reference;
	Mat3x2 referenceTransform;
	const ConvexPolygon* incident;
	Mat3x2 incidentTransform;
	Mat2 incidentRot;
	Vec2 normal;

	Vec2 edgeStart;
	Vec2 edgeEnd;

	Vec2 referenceFaceMidPoint;
	if (aS->separation > bS->separation) {
		reference = &a;
		incident = &b;
		normal = a.normals[aS->edgeIndex] * Mat2::rotate(aOrientation) * aS->separation;
		referenceFaceMidPoint = (a.verts[aS->edgeIndex] + a.verts[(aS->edgeIndex + 1) % a.verts.size()]) / 2.0f * Mat2::rotate(aOrientation) + aPos;
		/*min = a.normals[aS->edgeIndex] * Mat2::rotate(aOrientation) * aS->separation;
		edge = (a.verts[aS->edgeIndex] + a.verts[(aS->edgeIndex + 1) % a.verts.size()]) / 2.0f * Mat2::rotate(aOrientation) + aPos;*/
		referenceTransform = Mat3x2::rotate(aOrientation) * Mat3x2::translate(aPos);
		incidentTransform = Mat3x2::rotate(bOrientation) * Mat3x2::translate(bPos);
		incidentRot = Mat2::rotate(bOrientation);
		edgeStart = a.verts[aS->edgeIndex] * referenceTransform;
		edgeEnd = a.verts[(aS->edgeIndex + 1) % a.verts.size()] * referenceTransform;
	} else {
		reference = &b;
		incident = &a;
		normal = b.normals[bS->edgeIndex] * Mat2::rotate(bOrientation) * bS->separation;
		referenceFaceMidPoint = (b.verts[bS->edgeIndex] + b.verts[(bS->edgeIndex + 1) % b.verts.size()]) / 2.0f * Mat2::rotate(bOrientation) + bPos;
		/*min = b.normals[bS->edgeIndex] * Mat2::rotate(bOrientation) * bS->separation;
		edge = (b.verts[bS->edgeIndex] + b.verts[(bS->edgeIndex + 1) % b.verts.size()]) / 2.0f * Mat2::rotate(bOrientation) + bPos;*/
		referenceTransform = Mat3x2::rotate(bOrientation) * Mat3x2::translate(bPos);
		incidentTransform = Mat3x2::rotate(aOrientation) * Mat3x2::translate(aPos);
		incidentRot = Mat2::rotate(aOrientation);
		edgeStart = b.verts[bS->edgeIndex] * referenceTransform;
		edgeEnd = b.verts[(bS->edgeIndex + 1) % b.verts.size()] * referenceTransform;
	}

	i32 furthestPointOfIndidentInsideReference = 0;
	auto maxDistance = -std::numeric_limits<float>::infinity();
	for (usize i = 0; i < incident->verts.size(); i++) {
		auto d = dot(normal, incident->verts[i] * incidentTransform);
		if (d > maxDistance) {
			maxDistance = d;
			furthestPointOfIndidentInsideReference = i;
		}
	}

	auto getEdges = [](const ConvexPolygon* p, u32 i, const Mat3x2& t) -> std::pair<Vec2, Vec2> {
		return { p->verts[i] * t, p->verts[(i + 1) % p->verts.size()] * t };
	};

	auto drawEdge = [](const ConvexPolygon* p, u32 i, const Mat3x2& t) -> void {
		Debug::drawLine(p->verts[i] * t, p->verts[(i + 1) % p->verts.size()] * t, Vec3::RED);
	};

	/*Vec2 p = incident->verts[furthestPointOfIndidentInsideReference] * incidentTransform;
	Debug::drawPoint(p, Vec3::GREEN);*/

	auto face0Index = furthestPointOfIndidentInsideReference;
	auto face1Index = (furthestPointOfIndidentInsideReference - 1);
	if (face1Index < 0) {
		face1Index = incident->normals.size() - 1;
	}
	auto face0 = incident->normals[face0Index] * incidentRot;
	auto face1 = incident->normals[face1Index] * incidentRot;

	i32 face;
	if (dot(normal, face0) > dot(normal, face1)) {
		face = face0Index;
	} else {
		face = face1Index;
	}
	Debug::drawLine(edgeStart, edgeEnd, Vec3::GREEN);
	drawEdge(incident, face, incidentTransform);

	auto incidentEdges = getEdges(incident, face, incidentTransform);

	Line lineA{ edgeStart, edgeStart + normal };
	//Debug::drawRay(Vec2{ 0.0f }, lineA.n, Vec3::BLUE);

	Debug::drawLineSegment(LineSegment{ lineA, -0.5f, 0.5f });

	Line lineB{ edgeEnd, edgeEnd - normal };
	Debug::drawRay(Vec2{ 0.0f }, lineB.n, Vec3::BLUE);

	//Debug::drawLineSegment(LineSegment{ lineB, -0.5f, 0.5f });

	//Debug::drawPoint(incidentEdges.first, Vec3::RED);
	//Debug::drawPoint(incidentEdges.second, Vec3::RED);

	auto clipPoints = [](const std::pair<Vec2, Vec2>& points, const Line& line) -> std::pair<Vec2, Vec2> {
		auto clipPoint = [](Vec2 p, const Line& line, const Line& edgeLine) -> Vec2 {
			auto distanceFromLine = signedDistance(line, p);
			if (distanceFromLine > 0.0f) {
				auto x = line.intersection(edgeLine);
				if (!x.has_value()) {
					Debug::drawPoint(Vec2{ 0.0f }, Vec3::GREEN);
				}

				return *x;
				//auto h = p + distanceFromLine * line.n;
				//return h + signedDistance(edgeLine, h) * edgeLine.n;
			}
			return p;
		};

		Line edgeLine{ points.first, points.second };

		return {
			clipPoint(points.first, line, edgeLine),
			clipPoint(points.second, line, edgeLine)
		};
	};

	auto cliped = clipPoints(incidentEdges, lineA);
	cliped = clipPoints(cliped, lineB);

	std::vector<Vec2> points;
	Line incidentFaceLine{ edgeStart, edgeEnd };
	auto d = signedDistance(incidentFaceLine, cliped.first);
	if (d >= 0.0f) {
		points.push_back(cliped.first);
	}

	d = signedDistance(incidentFaceLine, cliped.second);
	if (d >= 0.0f) {
		points.push_back(cliped.second);
	}


	for (auto& p : points) {
		Debug::drawPoint(p, Vec3::RED);
	}



	/*Debug::drawPoint(cliped.first, Vec3::RED);
	Debug::drawPoint(cliped.second, Vec3::RED);*/

	Debug::drawRay(edge, min, Vec3::RED);
}

auto drawPolygon(const ConvexPolygon& polygon, Vec2 pos, float orientation) {
	const auto& [verts, normals] = polygon;
	const auto transform = Mat3x2::rotate(orientation) * Mat3x2::translate(pos);
	const auto rotation = Mat2::rotate(orientation);
	for (usize i = 0; i < verts.size() - 1; i++) {
		Debug::drawLine(verts[i] * transform, verts[i + 1] * transform);
	}
	Debug::drawLine(verts.back() * transform, verts[0] * transform);

	for (usize i = 0; i < verts.size() - 1; i++) {
		auto midPoint = (verts[i] * transform + verts[i + 1] * transform) / 2.0f;
		Debug::drawRay(midPoint, normals[i] * rotation * 0.05f);
	}
	auto midPoint = (verts.back() * transform + verts[0] * transform) / 2.0f;
	Debug::drawRay(midPoint, normals.back() * rotation * 0.05f);
}

auto TestDemo::update() -> void {
	Camera camera;

	Vec2 aPos{ camera.cursorPos() }; 
	static float aOrientation = 0.0f;
	if (Input::isKeyHeld(Keycode::A)) {
		aOrientation += 0.01f;
	}
	if (Input::isKeyHeld(Keycode::D)) {
		aOrientation -= 0.01f;
	}
	auto cornersA = BoxCollider{ BoxColliderEditor{ Vec2{ 0.1f } } }.getCorners(Vec2{ 0.0f }, 0.0f);
	std::vector<Vec2> verticesA;
	for (auto corner : cornersA) {
		verticesA.push_back(corner);
	}
	/*ConvexPolygon a{ verticesA };
	a.calculateNormals();*/
	auto a = makeRegularPolygon(4, 0.15f);

	Vec2 bPos{ 0.3f, 0.2f };
	float bOrientation = 0.0f;
	//auto cornersB = BoxCollider{ BoxColliderEditor{ Vec2{ 0.2f } } }.getCorners(Vec2{ 0.0f }, 0.0f);
	//std::vector<Vec2> verticesB;
	//for (auto corner : cornersB) {
	//	verticesB.push_back(corner);
	//}
	//ConvexPolygon b{ verticesB };
	//b.calculateNormals();
	auto b = makeRegularPolygon(4, 0.2f);


	drawPolygon(a, aPos, aOrientation);
	drawPolygon(b, bPos, bOrientation);

	collide(a, aPos, aOrientation, b, bPos, bOrientation);

	Renderer::update(camera);
}
