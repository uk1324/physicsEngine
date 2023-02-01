#include <game\levelFormat\levelData.hpp>
using namespace Json;

auto LevelBox::toJson() const -> Json::Value {
	auto result = Json::Value::emptyObject();
	result["size"] = { { "x", size.x }, { "y", size.y } };
	return result;
}

auto LevelBox::fromJson(const Json::Value& json) -> LevelBox {
	return LevelBox{
		.size = Vec2{ json.at("size").at("x").number(), json.at("size").at("y").number() },
	};
}

auto LevelCircle::toJson() const -> Json::Value {
	auto result = Json::Value::emptyObject();
	result["radius"] = Json::Value(radius);
	return result;
}

auto LevelCircle::fromJson(const Json::Value& json) -> LevelCircle {
	return LevelCircle{
		.radius = json.at("radius").number(),
	};
}

auto LevelBody::toJson() const -> Json::Value {
	auto result = Json::Value::emptyObject();
	result["pos"] = { { "x", pos.x }, { "y", pos.y } };
	result["orientation"] = Json::Value(orientation);
	result["vel"] = { { "x", vel.x }, { "y", vel.y } };
	result["angularVel"] = Json::Value(angularVel);
	result["mass"] = Json::Value(mass);
	result["rotationalInertia"] = Json::Value(rotationalInertia);
	result["coefficientOfFriction"] = Json::Value(coefficientOfFriction);
	result["collider"] = levelColliderToJson(collider);
	return result;
}

auto LevelBody::fromJson(const Json::Value& json) -> LevelBody {
	return LevelBody{
		.pos = Vec2{ json.at("pos").at("x").number(), json.at("pos").at("y").number() },
		.orientation = json.at("orientation").number(),
		.vel = Vec2{ json.at("vel").at("x").number(), json.at("vel").at("y").number() },
		.angularVel = json.at("angularVel").number(),
		.mass = json.at("mass").number(),
		.rotationalInertia = json.at("rotationalInertia").number(),
		.coefficientOfFriction = json.at("coefficientOfFriction").number(),
		.collider = levelColliderFromJson(json.at("collider")),
	};
}

auto LevelDistanceJoint::toJson() const -> Json::Value {
	auto result = Json::Value::emptyObject();
	result["bodyAIndex"] = Json::Value(bodyAIndex);
	result["bodyBIndex"] = Json::Value(bodyBIndex);
	result["distance"] = Json::Value(distance);
	return result;
}

auto LevelDistanceJoint::fromJson(const Json::Value& json) -> LevelDistanceJoint {
	return LevelDistanceJoint{
		.bodyAIndex = json.at("bodyAIndex").intNumber(),
		.bodyBIndex = json.at("bodyBIndex").intNumber(),
		.distance = json.at("distance").number(),
	};
}

