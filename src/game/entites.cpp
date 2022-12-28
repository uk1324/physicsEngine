#include <game/entitesData.hpp>
#include <utils/overloaded.hpp>

#define JSON(type) \
	[](const type& value) { return Json::Value{ { "type", #type }, { "value", value.toJson() } }; },

auto colliderToJson(const Collider& collider) -> Json::Value {
	return std::visit(overloaded{
		JSON(BoxCollider)
		JSON(CircleCollider)
	}, collider);
}

#define UNJSON(type) \
	if (json.at("type").string() == #type) { return type::fromJson(json.at("value")); }

auto jsonToCollider(const Json::Value& json) -> Collider {

	UNJSON(BoxCollider)
	else UNJSON(CircleCollider)

	ASSERT_NOT_REACHED();
	return Collider{ CircleColliderEditor{ 1.0f } };
}

auto displayColliderGui(const Collider& collider) -> void {

}

auto anchorToJson(const std::variant<Vec2, DistanceJointAnchorEditor>& anchor) -> Json::Value {
	return std::visit(overloaded{
		[](Vec2 v) { return Json::Value{ { "type", "Vec2" }, { "value", { { "x", Json::Value(v.x) }, { "y", Json::Value(v.y) } } } }; },
		JSON(DistanceJointAnchorEditor)
	}, anchor);
}

auto jsonToAnchor(const Json::Value& json) -> std::variant<Vec2, DistanceJointAnchorEditor> {
	if (json.at("type").string() == "Vec2") {
		auto& value = json.at("value");
		return Vec2{ value.at("x").floatNumber(), value.at("y").floatNumber() };
	}
	else UNJSON(DistanceJointAnchorEditor)
}

auto displayAnchorGui(const std::variant<Vec2, DistanceJointAnchorEditor>& anchor) -> void {

}