#include <game/demos/testingDemo.hpp>
#include <game/ent.hpp>
#include <math/polygon.hpp>
#include <utils/dbg.hpp>

#include <imgui/imgui.h>
using namespace ImGui;

auto TestingDemo::name() -> const char* {
	return "testing";
}

bool crossJoin = false;

auto TestingDemo::loadSettingsGui() -> void {
	Checkbox("cross join", &crossJoin);
}

struct Softbody {
	std::vector<BodyId> bodies;
	float initialVolume;
	auto volume() -> float {
		std::vector<Vec2> pos;
		for (const auto body : bodies) {
			pos.push_back(ent.body.get(body)->transform.pos);
		}
		return simplePolygonArea(pos);
		//return -simplePolygonSignedArea(pos);
	}
};

//namespace {
//	thread_local std::vector<Vec2> computeHullResult;
//}
//auto computeHull(Span<const Vec2> points, std::vector<Vec2>& result = computeHullResult) -> const std::vector<Vec2>& {
//	result.clear();
//
//	
//	std::vector<LineSegment> segments;
//	Vec2 current = points[0];
//	const auto start = 0;
//	int next = 1;
//	do {
//		const auto line = LineSegment{ current, points[next] };
//		int previous = points.size() - 1;
//		for (int i = 0; i < points.size(); previous = i, i++) {
//			int n = next - 1;
//			if (n < 0) {
//				n = points.size() - 1;
//			}
//			if (i == next || previous == n || i == n || previous == next) {
//				continue;
//			}
//			const auto l = LineSegment{ points[previous], points[i] };
//			if (const auto intersection = l.intersection(line)) {
//				current = *intersection;
//				Debug::drawPoint(current, Vec3::BLUE);
//				next = previous;
//			} else {
//				current = points[next];
//				next = next + 1;
//				if (next >= points.size()) {
//					next = 0;
//				}
//			}
//			result.push_back(current);
//		}
//		if (result.size() > 5 * points.size()) {
//			break;
//		}
//	} while (distance(current, points[0]) > 0.01f);
//	
//	//for (int i = 0; i < 3 && i < points.size(); i++) {
//	//	result.push_back(points[i]);
//	//}
//
//	//for (int i = 4; i < points.size(); i++) {
//	//	const auto& toInsert = points[i];
//	//	if (!isPointInPolygon(result, toInsert)) {
//	//		int closest = 0;
//	//		int secondClosest;
//	//		float closestDistance = std::numeric_limits<float>::infinity();
//	//		for (int j = 1; j < result.size(); j++) {
//	//			const auto d = distance(result[j], toInsert);
//	//			if (d < closestDistance) {
//	//				closestDistance = d;
//	//				secondClosest = closest;
//	//				closest = j;
//	//			}
//	//		}
//	//		//result.push_back(points[i]);
//	//		result.insert(result.begin() + closest, toInsert);
//	//		/*std::erase_if(result, [&](const Vec2& p) {
//	//			return isPointInPolygon(result, p);
//	//		});*/
//	//	}
//	//}
//
//	return result;
//}

std::vector<Softbody> softbodies;

auto join(BodyId a, BodyId b, bool spring = true) -> void {
	//spring = !spring;
	const auto d = distance(ent.body.get(a)->transform.pos, ent.body.get(b)->transform.pos);
	if (spring)
		ent.springJoint.create(SpringJoint{ a, b, d });
	else
		ent.distanceJoint.create(DistanceJoint{ a, b, d });
}

auto connect(const std::vector<BodyId> bodies) -> void {
	auto previous = bodies.back();
	for (auto body : bodies) {
		join(body, previous);
		previous = body;
	}
}

auto softbodyGrid(Vec2 pos) -> void {
	static constexpr Vec2T<i64> size{ 5, 30 };
	//static constexpr Vec2T<i64> size{ 2, 3 };
	BodyId grid[size.x][size.y];
	const auto scale = 0.25f;
	for (int x = 0; x < size.x; x++) {
		for (int y = 0; y < size.y; y++) {
			/*grid[x][y] = ent.body.create(Body{ Vec2(x, y) * scale + Vec2{ 1.0f }, CircleCollider{ 0.5f * scale }, false }).id;*/
			/*grid[x][y] = ent.body.create(Body{ Vec2(x, y) * scale + Vec2{ 1.0f }, CircleCollider{ 0.5f * scale }, false }).id;*/
			/*grid[x][y] = ent.body.create(Body{ Vec2(x, y) * scale + pos, CircleCollider{ 0.1f * scale }, false }).id;*/
			auto body = ent.body.create(Body{ Vec2(x, y) * scale + pos, CircleCollider{ 0.1f * scale }, false });
			/*body->mass /= 4.0f;*/
			body->mass /= 4.0f;
			grid[x][y] = body.id;
			/*grid[x][y] = ent.body.create(Body{ Vec2(x, y) * scale + Vec2{ 1.0f }, BoxCollider{ Vec2{ scale } }, false }).id;*/
		}
	}

	for (int x = 0; x < size.x; x++) {
		for (int y = 0; y < size.y; y++) {
			//if (x != size.x - 1 && y != size.y - 1) {
			//	softbodies.push_back(Softbody{
			//		.bodies = {
			//			grid[x][y], grid[x + 1][y], grid[x + 1][y + 1]
			//		}
			//	});
			//	softbodies.back().initialVolume = softbodies.back().volume();

			//	softbodies.push_back(Softbody{
			//		.bodies = {
			//			grid[x + 1][y], grid[x][y + 1], grid[x + 1][y + 1]
			//		}
			//		});
			//	softbodies.back().initialVolume = softbodies.back().volume();
			//}

			if (x != 0) {
				join(grid[x - 1][y], grid[x][y]);
			}

			if (y != 0) {
				join(grid[x][y - 1], grid[x][y]);
			}

			if (crossJoin) {
				if (x != 0 && y != 0) {
					join(grid[x - 1][y - 1], grid[x][y], false);
				}
				if (x != 0 && y != size.y - 1) {
					join(grid[x - 1][y + 1], grid[x][y], false);
				}
			}
		}
	}

	std::vector<BodyId> outer;
	for (int x = 0; x < size.x; x++) {
		outer.push_back(grid[x][0]);
	}
	for (int y = 1; y < size.y; y++) {
		outer.push_back(grid[size.x - 1][y]);
	}
	for (int x = size.x - 2; x >= 0; x--) {
		outer.push_back(grid[x][size.y - 1]);
	}
	for (int y = size.y - 2; y >= 1; y--) {
		outer.push_back(grid[0][y]);
	}
	softbodies.push_back(Softbody{
		.bodies = std::move(outer)
	});
	softbodies.back().initialVolume = softbodies.back().volume() * 1.05f;
}


auto circle(Vec2 pos, float size, bool offset = false) -> std::vector<BodyId> {
	int steps = 30;
	std::vector<BodyId> outer;
	for (int i = 0; i < steps; i++) {
		auto angle = i * TAU<float> / steps;
		if (offset) {
			angle += TAU<float> / steps / 2.0f;
		}
		auto body = ent.body.create(Body{ Vec2::oriented(angle) * size + pos, CircleCollider{ 0.5f * 0.25f }, false }).id;
		//auto body = ent.body.create(Body{ Vec2::oriented(angle) * size + pos, BoxCollider{ Vec2{ 0.25f } }, false }).id;
		outer.push_back(body);

	}
	return outer;
}

auto softbodyCircle() -> void {
	auto makeBall = [&](Vec2 pos, float size) {
		auto outer = circle(pos, 1.0f * size);
		connect(outer);

		auto inner = circle(pos, 0.7f * size);
		connect(inner);

		auto get = [](const std::vector<BodyId>& body, int i) {
			if (i >= body.size()) {
				i = 0;
			}
			return body[i];
		};

		for (int i = 0; i < inner.size(); i++) {
			join(get(outer, i), get(inner, i));
			join(get(outer, i), get(inner, i + 1));
			join(get(outer, i + 1), get(inner, i));
		}
	};
	for (int i = 0; i < 25; i++) {
		makeBall(Vec2{ 0.0f, 5.0f + i * 5.0f }, 1.0f);
	}
}

auto TestingDemo::load() -> void {
	/*softbodyGrid();
	softbodyCircle();*/
	/*body.bodies = circle(Vec2{ 0.0f, 5.0f }, 2.0f);
	body.initialVolume = body.volume();
	connect(body.bodies);*/

	//softbodyGrid(Vec2{ 0.0f, 1.0f });
	//softbodyGrid(Vec2{ 4.0f, 1.0f });

	//const auto a = ent.body.create(Body{ Vec2{ 5.0f, 5.0f }, CircleCollider{ 1.0f }, false });
	//const auto b = ent.body.create(Body{ Vec2{ 5.0f, 5.0f }, CircleCollider{ 1.0f }, false });
	//for (const auto body : ent.body) {
	//	ent.collisionsToIgnore.insert({ a.id, body.id });
	//	ent.collisionsToIgnore.insert({ b.id, body.id });
	//}

	ent.body.create(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 200.0f, 100.0f } }, true });
}

float force = 50.0f;
auto TestingDemo::settingsGui() -> void {
	InputFloat("force", &force);
	Text("rest: %g", softbodies[0].initialVolume);
	Text("now: %g", softbodies[0].volume());
}

#include <numeric>


struct ShapeMatchingSoftbody {
	std::vector<Vec2> shape;
	std::vector<BodyId> bodies;
	float lastAngle = 0.0f;

	auto matchShape() -> std::vector<Vec2> {
		std::vector<Vec2> verticesNow;
		for (auto& b : bodies) {
			verticesNow.push_back(ent.body.get(b)->transform.pos);
		}

		std::vector<Vec2> orientedVertices;
		const auto centroid = std::accumulate(verticesNow.begin(), verticesNow.end(), Vec2{ 0.0f }) / verticesNow.size();
		//Debug::drawPoint(centroid, Vec3::RED);
		Mat2 m;
		auto sqrtM = [](const Mat2& m) {
			const auto delta = m[0][0] * m[1][1] - m[1][0] * m[0][1];
			const auto tau = m[0][0] + m[1][1];
			const auto s = sqrt(delta);
			const auto t = sqrt(tau);
			/*return Mat2{ Vec2{ m[0][0] + s, m[0][1] }, Vec2{ m[1][0], m[1][1] + s } } * (1.0f / t);*/
			return Mat2{ Vec2{ m[0][0] + s, m[0][1] }, Vec2{ m[1][0], m[1][1] + s } } * (1.0f / t);
		};

		Mat2 pq{ Vec2{ 0.0f }, Vec2{ 0.0f } };
		//Mat2 pq{ Vec2{ 0.0f }, Vec2{ 0.0f } };
		
		for (int i = 0; i < verticesNow.size(); i++) {
			const auto p = (verticesNow[i] - centroid);
			const auto q = shape[i];
			/*Mat2 pq0{ Vec2{ p.x * q.x, p.x * q.y }, Vec2{ p.y * q.x, p.y * q.y } };*/
			Mat2 pq0{ Vec2{ p.x * q.x, p.y * q.x }, Vec2{ p.x * q.y, p.y * q.y } };
			pq = Mat2{ pq.x() + pq0.x(), pq.y() + pq0.y() };
			//Mat2 qq{ Vec2{ q.x * q.x, q.x * q.y }, Vec2{ q.y * q.x, q.y * q.y } };

			//const auto c = a * b.inversed();

			
			
			/*angle += dot(p, q);*/	
			/*const auto cos = std::clamp(, , -1.0f, 1.0f);*/

			/*const auto cos = std::clamp(dot((centroid - verticesNow[i]).normalized(), shape[i].normalized()), -1.0f, 1.0f);
			const auto sin = std::clamp(dot((centroid - verticesNow[i]).normalized().rotBy90deg(), shape[i].normalized()), -1.0f, 1.0f);*/
			/*ASSERT(!isnan(cos));
			ASSERT(!isnan(sin));
			auto x = atan2(sin, cos);
			ASSERT(!isnan(x));
			angle += atan2(sin, cos);			*/
			/*const auto a = (centroid - verticesNow[i]);
			const auto b = shape[i].normalized();
			angle += b.angle() - a.angle();*/
			//const auto sin = std::clamp(dot((verticesNow[i] - centroid).normalized().rotBy90deg(), shape[i].normalized()), -1.0f, 1.0f);
			//angle += abs(atan2(sin, cos));
			/*const auto cos = std::clamp(dot((verticesNow[i] - centroid).normalized(), shape[i].normalized()), -1.0f, 1.0f);
			angle += acos(cos);*/
		}
		const auto s = sqrtM(pq.transposed() * pq);
		auto r = pq * s.inversed();
		r = Mat2{ r.x().normalized(), r.y().normalized() };


		//float a = 0.0f;
		//for (const auto& v : shape) {
		//	a += dot(v, v);
		//}
		///*angle /= shape.size();*/
		//angle /= a;
		/*for (const auto vertex : shape) {
			orientedVertices.push_back(vertex * Rotation{ angle } + centroid);
		}*/
		//for (const auto vertex : shape) {
		//	/*orientedVertices.push_back(vertex * Rotation{ -angle } + centroid);*/
		//	orientedVertices.push_back(vertex * Rotation{ angle } + centroid);
		//}
		float angle = 0.0f;
		for (int i = 0; i < verticesNow.size(); i++) {
			const auto p = (verticesNow[i] - centroid);
			const auto q = shape[i];
			const auto sin = det(p, q);
			/*const auto cos = std::clamp(dot(p.normalized(), q.normalized()), -1.0f, 1.0f);*/
			const auto cos = dot(p, q);
			angle += atan2(sin, cos);
			//angle += acos(cos);
		}
		angle /= verticesNow.size();

		auto angleDifference = angle - lastAngle;
		if (angleDifference >= 180.0f) {
			if (angleDifference < 0.0f)
				angleDifference += 360.0f;
			else
				angleDifference -= 360.0f;
		}


		for (const auto vertex : shape) {
			//orientedVertices.push_back(vertex * Rotation{ -angle } + centroid);
			orientedVertices.push_back(vertex * r.inversed() + centroid);
			/*orientedVertices.push_back(vertex * r.inversed() + centroid);*/
		}
		return orientedVertices;

		lastAngle = angle;
		for (const auto body : bodies) {
			auto b = ent.body.get(body);
			const auto rotation = (b->transform.pos - centroid).normalized().rotBy90deg();
			b->vel -= rotation * dot(b->vel, rotation);
			b->vel += angleDifference * rotation * (b->transform.pos - centroid).length();
		}
	}
};

std::vector<ShapeMatchingSoftbody> shapeMatchingSoftbodies;

std::vector<Vec2> vertices;

#include <game/input.hpp>

auto TestingDemo::update(const DemoData& data) -> void {

	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		vertices.push_back(data.cursorPos);
	}

	if (vertices.size() == 1) {
		Debug::drawPoint(vertices[0]);
	} else if (vertices.size() > 1) {
		for (int i = 0; i < vertices.size() - 1; i++) {
			Debug::drawLine(vertices[i], vertices[i + 1]);
		}
	}

	if (Input::isKeyDown(Keycode::C) && vertices.size() != 0) {
		ShapeMatchingSoftbody body{ .shape = vertices };
		for (const auto& vertex : body.shape) {
			body.bodies.push_back(ent.body.create(Body{ vertex, CircleCollider{ 0.1f } }).id);
		}
		const auto centroid = std::accumulate(body.shape.begin(), body.shape.end(), Vec2{ 0.0f }) / body.shape.size();
		for (auto& vertex : body.shape) {
			vertex -= centroid;
		}
		vertices.clear();
		shapeMatchingSoftbodies.push_back(std::move(body));
		const auto& b = shapeMatchingSoftbodies.back();

		/*int previousI = b.bodies.size() - 1;
		for (int i = 0; i < b.bodies.size(); previousI = i, i++) {
			const auto d = distance(b.shape[i], b.shape[previousI]);
			ent.distanceJoint.create(DistanceJoint{ b.bodies[i], b.bodies[previousI], d });
		}*/

	}

	for (auto& body : shapeMatchingSoftbodies) {
		//Debug::drawLines(body.matchShape(), Vec3::RED);
		std::vector<Vec2> verticesNow;
		for (auto& b : body.bodies) {
			verticesNow.push_back(ent.body.get(b)->transform.pos);
		}
		Debug::drawLines(verticesNow);
	}

	for (const auto& softbody : softbodies) {
		
		std::vector<Vec2> pos;
		for (const auto body : softbody.bodies) {
			pos.push_back(ent.body.get(body)->transform.pos);
		}
		const auto& newPos = complexPolygonOutline(pos);
		int previous = newPos.size() - 1;
		for (int i = 0; i < newPos.size(); previous = i, i++) {
			const auto n = (newPos[i] - newPos[previous]).rotBy90deg().normalized();
			Debug::drawLine(newPos[i], newPos[previous], Vec3::RED);
		}
		//int previous = softbody.bodies.size() - 1;
		//for (int i = 0; i < softbody.bodies.size(); previous = i, i++) {
		//	const auto a = ent.body.get(softbody.bodies[i])->transform.pos;
		//	const auto b = ent.body.get(softbody.bodies[previous])->transform.pos;
		//	const auto n = (a - b).rotBy90deg().normalized();
		//	Debug::drawLine(a + n * 0.1f, b + n * 0.1f, Vec3::RED);
		//}
	}
}

auto TestingDemo::physicsStep() -> void {

	for (auto& body : softbodies) {
		std::vector<Vec2> pos;
		for (const auto body : body.bodies) {
			pos.push_back(ent.body.get(body)->transform.pos);
		}
		auto volumeDifference = body.initialVolume - body.volume();
		int previous = body.bodies.size() - 1;
		for (int i = 0; i < body.bodies.size(); previous = i, i++) {
			ent.body.get(body.bodies[i])->angularVel = 0.0f;
			const auto length = (pos[i] - pos[previous]).length();
			const auto n = (pos[i] - pos[previous]).normalized().rotBy90deg();
			//Debug::drawRay((pos[i] + pos[previous]) / 2.0f, n);
			ent.body.get(body.bodies[i])->vel += n * volumeDifference * length * force;
			ent.body.get(body.bodies[previous])->vel += n * volumeDifference * length * force;
		}
	}

	for (auto& body : shapeMatchingSoftbodies) {
		const auto& shape = body.matchShape();
		for (int i = 0; i < shape.size(); i++) {
			const auto d = (shape[i] - ent.body.get(body.bodies[i])->transform.pos);
			ent.body.get(body.bodies[i])->vel += d * 5.0f;
			ent.body.get(body.bodies[i])->angularVel = 0.0f;
			//ent.body.get(body.bodies[i])->vel -= d.normalized() * dot(ent.body.get(body.bodies[i])->vel, d.normalized()) * 0.5f;
		}
	}
}
