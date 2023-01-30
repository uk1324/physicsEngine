#include <game/ent.hpp>

auto Entites::update() -> void {
	body.update();
	distanceJoint.update();
}

auto Entites::reset() -> void {
	body.reset();
	distanceJoint.reset();
}

Entites ent;