#include <demos/triangulationDemo.hpp>
#include <game/debug.hpp>
#include <engine/input.hpp>
#include <math/mat2.hpp>

TriangulationDemo::TriangulationDemo() {
}

auto TriangulationDemo::update(Gfx& gfx, Renderer& renderer) -> void {
	Camera camera;


	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		points.push_back(camera.cursorPos());
	}

	Debug::drawLines(points);

	/*Vec2 a{ -1, -1 }, b{ 1, 1 };

	Vec2 d{ -1, 0.5 };

	Line l0{ a, b };
	Line l1{ camera.cursorPos(), d };*/

	/*Debug::drawLine(a, b);
	Debug::drawLine(camera.cursorPos(), d);*/

	/*auto l = l0.intersection(l1);
	if (l.has_value()) {
		Debug::drawPoint(*l);
	}*/


	if (points.size() >= 2 && Input::isKeyDown(Keycode::A)) {
		auto aabb = Aabb::fromCorners(points[0], points[1]);
		for (usize i = 2; i < points.size(); i++) {
			aabb = aabb.extended(points[i]);
		}
		Debug::drawAabb(aabb);

		std::vector<LineSegment> lineSegments;

		if (points.size() >= 2) {
			lineSegments.push_back(LineSegment{ points[0], points[1] });
			for (usize i = 1; i < points.size() - 1; i++) {
				lineSegments.push_back(LineSegment{ points[i], points[i + 1] });
			}
			if (points.size() > 2) {
				lineSegments.push_back(LineSegment{ points[points.size() - 1], points[0] });
			}
		}

		auto isPointInsideShape = [&](Vec2 p) -> bool {
			auto current = Vec2{ aabb.min.x - 0.2f, (aabb.min.y + aabb.max.y) / 2.0f };
			std::optional<LineSegment*> toIgnore;
			usize hitCount = 0;
			for (;;) {
				Debug::drawPoint(current);
				std::optional<Vec2> closestHitPoint;
				std::optional<LineSegment*> closestHitSegment;
				float closestHitPointDistance = std::numeric_limits<float>::infinity();
				for (auto& segment : lineSegments) {
					if (&segment == toIgnore)
						continue;

					auto hit = segment.raycastHit(current, p);
					if (hit.has_value()) {
						Debug::drawPoint(*hit, Vec3::RED);
						if (closestHitPoint.has_value()) {
							if (const auto d = distance(*hit, current); d < closestHitPointDistance) {
								closestHitPointDistance = d;
								closestHitPoint = *hit;
								closestHitSegment = &segment;
							}
						} else {
							closestHitPoint = *hit;
							closestHitPointDistance = distance(*closestHitPoint, current);
							closestHitSegment = &segment;
						}
					}
				}

				if (!closestHitPoint.has_value()) {
					// Ray could also be collinear.
					return hitCount % 2 == 1;
				}

				toIgnore = closestHitSegment;
				Debug::drawLineSegment(**closestHitSegment, Vec3::BLUE);
				Debug::drawLine(current, *closestHitPoint);
				current = *closestHitPoint;
				hitCount++;

				if (distance(*closestHitPoint, p) < 0.001f) {
					return hitCount % 2 == 1;
				}
			}
		};

		if (isPointInsideShape(camera.cursorPos())) {
			Debug::drawPoint(camera.cursorPos(), Vec3::RED);
		}
	}

	renderer.update(gfx, camera);
}
