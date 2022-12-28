#include <game/debug.hpp>
#include <math/mat2.hpp>

auto Debug::update() -> void {
	lines.clear();
	circles.clear();
	points.clear();
	circleColliders.clear();
	hollowCircles.clear();
	parabolas.clear();
}

auto Debug::drawLine(Vec2 start, Vec2 end, const Vec3& color) -> void {
	lines.push_back({ start, end, color });
}

auto Debug::drawLineSegment(const LineSegment& lineSegment, const Vec3& color) -> void {
	const auto corners = lineSegment.getCorners();
	Debug::drawLine(corners[0], corners[1], color);
}

auto Debug::drawRay(Vec2 start, Vec2 ray, const Vec3& color) -> void {
	lines.push_back({ start, start + ray, color });
}

auto Debug::drawCircle(Vec2 pos, float radius, const Vec3& color) -> void {
	circles.push_back({ pos, radius, color });
}

auto Debug::drawHollowCircle(Vec2 pos, float radius, const Vec3& color) -> void {
	hollowCircles.push_back({ pos, radius, color });
}

auto Debug::drawCircleCollider(Vec2 pos, float radius, float orientation, const Vec3& color) -> void {
	circleColliders.push_back({ { pos, radius, color }, orientation });
}

auto Debug::drawPoint(Vec2 pos, const Vec3& color) -> void {
	points.push_back({ pos, 0.01f, color });
}

auto Debug::drawLines(Span<const Vec2> lines, const Vec3& color) -> void {
	if (lines.size() < 2)
		return;

	drawLine(lines[0], lines[1], color);

	if (lines.size() == 2)
		return;

	for (usize i = 1; i < lines.size() - 1; i++)
		drawLine(lines[i], lines[i + 1]);
	drawLine(lines.back(), lines[0], color);
}

auto Debug::drawAabb(const Aabb& aabb, const Vec3& color) -> void {
	const auto v1 = Vec2{ aabb.max.x, aabb.min.y };
	const auto v3 = Vec2{ aabb.min.x, aabb.max.y };
	Debug::drawLine(aabb.min, v1, color);
	Debug::drawLine(v1, aabb.max, color);
	Debug::drawLine(aabb.max, v3, color);
	Debug::drawLine(v3, aabb.min, color);
}

auto Debug::drawParabola(float a, Vec2 pos, const Vec3& color) -> void {
	parabolas.push_back({ a, pos, color });
}

auto Debug::drawBox(Vec2 pos, float orientation, Vec2 size, const Vec3& color) -> void {
	const auto rotate = Mat2::rotate(orientation);
	// @Performance: Could just use the basis from the rotate matrix. Or even better precompute the matrix because it is used in a lot of places.
	const auto edgeX = Vec2{ size.x, 0.0f } * rotate;
	const auto edgeY = Vec2{ 0.0f, size.y } * rotate;
	const auto vertex1 = (size / 2.0f) * rotate + pos;
	const auto vertex2 = vertex1 - edgeX;
	const auto vertex3 = vertex2 - edgeY;
	const auto vertex4 = vertex3 + edgeX;
	Debug::drawLine(vertex1, vertex2, color);
	Debug::drawLine(vertex2, vertex3, color);
	Debug::drawLine(vertex3, vertex4, color);
	Debug::drawLine(vertex4, vertex1, color);
}

auto Debug::drawCollider(const Collider& collider, Vec2 pos, float orientation, const Vec3& color) -> void {
	if (const auto box = std::get_if<BoxCollider>(&collider)) Debug::drawBox(pos, orientation, box->size, color);
	else if (const auto circle = std::get_if<CircleCollider>(&collider)) Debug::drawCircleCollider(pos, circle->radius, orientation, color);
	else ASSERT_NOT_REACHED();
}

std::vector<Debug::Line> Debug::lines;
std::vector<Debug::Circle> Debug::circles;
std::vector<Debug::Point> Debug::points;
std::vector<Debug::OrientedCircle> Debug::circleColliders;
std::vector<Debug::Circle> Debug::hollowCircles;
std::vector<Debug::Parabola> Debug::parabolas;