#include <game/trail.hpp>
#include <game/ent.hpp>
#include <engine/debug.hpp>

auto Trail::update() -> void {
	if (!ent.body.isAlive(body)) {
		ent.trail.destroy(*this);
		return;
	}
	const auto& b = ent.body.get(body);
	if (!b.has_value())
		return;

	const auto pos = b->transform.pos + anchor * b->transform.rot;
	if (history.size() == 0 || distance(pos, history.back()) > 0.02f) {
		history.push_back(pos);
	}
	while (history.size() > maxHistorySize) {
		history.pop_front();
	}
}

auto Trail::draw() const -> void {
	if (history.size() == 0)
		return;

	const auto& b = ent.body.get(body);
	if (!b.has_value())
		return;
	Debug::drawPoint(history.back(), color);
	Debug::drawLine(history.back(), b->transform.pos, color / 2.0f);

	if (history.size() <= 1)
		return;
	
	for (usize i = 0; i < history.size() - 1; i++)
		Debug::drawLine(history[i], history[i + 1], color);
	
}
