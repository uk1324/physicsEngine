#include <game/entitesData.hpp>

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

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
	return Collider{};
}

auto displayColliderGui(const Collider& collider) -> void {

}