#include <game/debug.hpp>

auto Debug::update() -> void {
	lines.clear();
}

auto Debug::drawLine(Vec2 start, Vec2 end, const Vec3& color) -> void {
	lines.push_back({ start, end, color });
}

std::vector<Debug::Line> Debug::lines;