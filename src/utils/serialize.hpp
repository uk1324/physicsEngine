#pragma once

#include <../thirdParty/json/JsonValue.hpp>

template<typename T>
auto vecToJson(const std::vector<T>& v) -> Json::Value {
	auto result = Json::Value::emptyArray();
	auto& array = result.array();
	for (const auto& i : v) {
		array.push_back(i.toJson());
	}
	return result;
}

template<typename T>
auto vecFromJson(const Json::Value& v) -> std::vector<T> {
	const auto& array = v.array();
	std::vector<T> result;
	for (const auto& i : array) {
		result.push_back(T::fromJson(i));
	}
	return result;
}
/*auto toJson = [](Vec2 v) -> Json::Value {
	return { { "x", v.x }, { "y", v.y } };
};*/
