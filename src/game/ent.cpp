#include <game/ent.hpp>

auto Entites::update() -> void {
	body.update();
	distanceJoint.update();
	trail.update();

		// TODO: Make the joint do this in the desctructor. There might be some issues with destructors of classes like body trying to delete bodies. Because it would add to a list that is being iterated. This shouldn't be an issues in this case though because it would be removing from revoluteJointsWithIgnoredCollisions while iterating over distance joints to destroy.
	std::erase_if(revoluteJointsWithIgnoredCollisions, [this](const auto& it) {
		const auto& [joint, bodyPair] = it;
		if (!ent.distanceJoint.isAlive(joint)) {
			collisionsToIgnore.erase(bodyPair);
			return true;
		}
		return false;
	});
}

auto Entites::reset() -> void {
	body.reset();
	distanceJoint.reset();
	collisionsToIgnore.clear();
	trail.reset();
	revoluteJointsWithIgnoredCollisions.clear();
}

Entites ent;