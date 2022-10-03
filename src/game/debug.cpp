#include <game/debug.hpp>

auto Debug::update() -> void {
	lines.clear();
	circles.clear();
}

auto Debug::drawLine(Vec2 start, Vec2 end, const Vec3& color) -> void {
	lines.push_back({ start, end, color });
}

auto Debug::drawCircle(Vec2 pos, float radius, const Vec3& color) -> void {
	circles.push_back({ pos, radius, color });
}

auto Debug::drawPoint(Vec2 pos, const Vec3& color) -> void {
	circles.push_back({ pos, 0.01f, color });
}

std::vector<Debug::Line> Debug::lines;
std::vector<Debug::Circle> Debug::circles;