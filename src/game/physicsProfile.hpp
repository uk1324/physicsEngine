#pragma once

// could add a counter for how many collisions were checked, but I don't know if this information is useful. Also I don't know how would it work in case of multistepping.
struct PhysicsProfile {
	float collideUpdateBvh = 0.0f;
	float collideDetectCollisions = 0.0f;
	float collideTotal = 0.0f;
	float solveTotal = 0.0f;
	float solvePrestep = 0.0f;
	float solveVelocities = 0.0f;
	float total = 0.0f;
};