#include <game/entitesData.hpp>
#include <utils/overloaded.hpp>

auto colliderToJson(const Collider& collider) -> Json::Value {
#define JSON(type) \
	[](const type& box) { return Json::Value{ { "type", #type }, { "collider", box.toJson() } }; },
	return std::visit(overloaded{
		JSON(BoxCollider)
		JSON(CircleCollider)
	}, collider);

	ASSERT_NOT_REACHED();
	return Json::Value::null();
}

auto jsonToCollider(const Json::Value& json) -> Collider {
#define UNJSON(type) \
	if (json.at("type").string() == #type) { return type::fromJson(json.at("collider")); }

	UNJSON(BoxCollider)
	else UNJSON(CircleCollider)

	ASSERT_NOT_REACHED();
	return Collider{ CircleColliderEditor{ 1.0f } };
}

auto displayColliderGui(const Collider& collider) -> void {

}