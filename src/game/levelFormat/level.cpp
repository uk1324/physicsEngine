#include <game/levelFormat/levelData.hpp>
#include <utils/overloaded.hpp>

auto LevelConvexPolygon::toJson() const -> Json::Value {
	auto json = Json::Value::emptyObject();
	json["verts"] = Json::Value::emptyArray();
	auto& vertices = json["verts"].array();
	for (auto& vert : verts) {
		vertices.push_back({ { "x", vert.x }, { "y", vert.y } });
	}
	return json;
}

auto LevelConvexPolygon::fromJson(const Json::Value& json) -> LevelConvexPolygon {
	LevelConvexPolygon polygon;
	for (const auto& vert : json.at("verts").array()) {
		polygon.verts.push_back(Vec2{ vert.at("x").floatNumber(), vert.at("y").floatNumber() });
	}
	return polygon;
}

auto levelColliderToJson(const LevelCollider& collider) -> Json::Value {

#define JSON(type) \
	[](const type& value) { return Json::Value{ { "type", #type }, { "value", value.toJson() } }; },

	return std::visit(overloaded{
		JSON(LevelBox)
		JSON(LevelCircle)
		JSON(LevelConvexPolygon)
	}, collider);

#undef JSON
}

auto levelColliderFromJson(const Json::Value& collider) -> LevelCollider{
#define UNJSON(type) \
	if (collider.at("type").string() == #type) { return type::fromJson(collider.at("value")); }

	UNJSON(LevelBox)
	UNJSON(LevelCircle)
	UNJSON(LevelConvexPolygon)

#undef UNJSON
}
