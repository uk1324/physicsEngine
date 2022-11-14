#include <game/entities.hpp>

//std::vector<CircleEntity> circleEntites;
std::vector<LineEntity> lineEntites;
std::vector<ConvexPolygonEntity> convexPolygonEntites;

PhysicsInfo::PhysicsInfo(const PhysicsMaterial* MATERIAL, float mass, BodyType bodyType, float rotationalInteria)
	: vel{ Vec2{ 0.0f } }
	, angularVel{ 0.0f }
	, MATERIAL{ MATERIAL }
	, invMass{ 1.0f / mass }
	, bodyType{ bodyType }
	, rotationalInteria{ rotationalInteria } {}

