#include <demos/testDemo.hpp>
#include <game/collision/collision.hpp>
#include <game/debug.hpp>
#include <engine/input.hpp>
#include <math/mat2.hpp>

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

struct Separation {
	float separation;
	i32 edgeIndex;
};

#include <math/utils.hpp>

static auto makeRegularPolygon(i32 sides, float radius) -> ConvexPolygon {
	ConvexPolygon polygon;
	const auto angleStep = TAU<float> / sides;
	for (i32 i = 0; i < sides; i++) {
		polygon.verts.push_back(Vec2::oriented(angleStep * i) * radius);
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
			Debug::drawPoint(v);
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
			Debug::drawPoint(v);
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
			Debug::drawPoint(a.normals[nI] * aRot * minA, Vec3::RED);
			Debug::drawPoint(a.normals[nI] * aRot * maxA, Vec3::RED);
			Debug::drawPoint(a.normals[nI] * aRot * minB, Vec3::GREEN);
			Debug::drawPoint(a.normals[nI] * aRot * maxB, Vec3::GREEN);
			Debug::drawRay(edge, min, Vec3::GREEN);
			if (Input::isKeyDown(Keycode::G)) {
				__debugbreak();
			}
			return std::nullopt;
		}

		/*if (minA <= maxB && maxA >= minB) {
			auto separation = minB - maxA;
			if (separation > maxSeparation) {
				maxSeparation = separation;
				maxSeparationNormal = nI;
			}
		} else if (!(((minA > minB) && (maxA < maxB)) || ((minB > minA) && (maxB < maxA)))) {
			auto min = a.normals[nI] * aRot * 0.1f;
			auto edge = (a.verts[nI] + a.verts[(nI + 1) % a.verts.size()]) / 2.0f * aRot * 0.1f + aPos;
			Debug::drawRay(edge, min, Vec3::GREEN);
			return std::nullopt;
		}*/
	}

	return Separation{
		maxSeparation,
		maxSeparationNormal
	};
}

//struct Transform {
//
//};

static auto collide(const ConvexPolygon& a, Vec2 aPos, float aOrientation, const ConvexPolygon& b, Vec2 bPos, float bOrientation) -> void {
	Vec2 min;
	Vec2 edge;

	auto aS = minSeparation(a, aPos, Mat2::rotate(aOrientation), b, bPos, Mat2::rotate(bOrientation));
	if (!aS.has_value())
		return;
	auto bS = minSeparation(b, bPos, Mat2::rotate(bOrientation), a, aPos, Mat2::rotate(aOrientation));
	if (!bS.has_value())
		return;
	if (aS->separation > bS->separation) {
		min = a.normals[aS->edgeIndex] * Mat2::rotate(aOrientation) * aS->separation;
		edge = (a.verts[aS->edgeIndex] + a.verts[(aS->edgeIndex + 1) % a.verts.size()]) / 2.0f * Mat2::rotate(aOrientation) + aPos;
	} else {
		min = b.normals[bS->edgeIndex] * Mat2::rotate(bOrientation) * bS->separation;
		edge = (b.verts[bS->edgeIndex] + b.verts[(bS->edgeIndex + 1) % b.verts.size()]) / 2.0f * Mat2::rotate(bOrientation) + bPos;
	}

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

auto TestDemo::update(Gfx& gfx, Renderer& renderer) -> void {
	Camera camera;

	Vec2 aPos{ camera.cursorPos() }; 
	static float aOrientation = 0.0f;
	if (Input::isKeyHeld(Keycode::A)) {
		aOrientation += 0.01f;
	}
	auto cornersA = BoxCollider{ BoxColliderEditor{ Vec2{ 0.1f } } }.getCorners(Vec2{ 0.0f }, 0.0f);
	std::vector<Vec2> verticesA;
	for (auto corner : cornersA) {
		verticesA.push_back(corner);
	}
	/*ConvexPolygon a{ verticesA };
	a.calculateNormals();*/
	auto a = makeRegularPolygon(3, 0.2f);

	Vec2 bPos{ 0.3f, 0.2f };
	float bOrientation = 0.0f;
	//auto cornersB = BoxCollider{ BoxColliderEditor{ Vec2{ 0.2f } } }.getCorners(Vec2{ 0.0f }, 0.0f);
	//std::vector<Vec2> verticesB;
	//for (auto corner : cornersB) {
	//	verticesB.push_back(corner);
	//}
	//ConvexPolygon b{ verticesB };
	//b.calculateNormals();
	auto b = makeRegularPolygon(6, 0.2f);

	drawPolygon(a, aPos, aOrientation);
	drawPolygon(b, bPos, bOrientation);

	collide(a, aPos, aOrientation, b, bPos, bOrientation);

	renderer.update(gfx, camera);
}
