#include <game/debug.hpp>

auto Debug::update() -> void {
	lines.clear();
	circles.clear();
	emptyCircles.clear();
}

auto Debug::drawLine(Vec2 start, Vec2 end, const Vec3& color) -> void {
	lines.push_back({ start, end, color });
}

auto Debug::drawRay(Vec2 start, Vec2 ray, const Vec3& color) -> void {
	lines.push_back({ start, start + ray, color });
}

auto Debug::drawCircle(Vec2 pos, float radius, const Vec3& color) -> void {
	circles.push_back({ pos, radius, color });
}

auto Debug::drawEmptyCircle(Vec2 pos, float radius, float orientation, const Vec3& color) -> void {
	emptyCircles.push_back({ { pos, radius, color }, orientation });
}

auto Debug::drawPoint(Vec2 pos, const Vec3& color) -> void {
	circles.push_back({ pos, 0.01f, color });
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

std::vector<Debug::Line> Debug::lines;
std::vector<Debug::Circle> Debug::circles;
std::vector<Debug::OrientedCircle> Debug::emptyCircles;