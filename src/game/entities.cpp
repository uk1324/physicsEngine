#include <game/entities.hpp>

std::vector<CircleEntity> circleEntites;
std::vector<LineEntity> lineEntites;
std::vector<ConvexPolygonCollider> convexPolygonEntites;

PhysicsInfo::PhysicsInfo(const PhysicsMaterial* material, float mass) 
	: vel{ Vec2{ 0.0f } }
	, angularVel{ 0.0f }
	, material{ material }
	, invMass{ 1.0f / mass } {}
