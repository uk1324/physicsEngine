#include <game\gameSettingsData.hpp>
#include <utils/serialize.hpp>
using namespace Json;

auto GameSettings::toJson() const -> Json::Value {
	auto result = Json::Value::emptyObject();
	result["levelLoadedBeforeClosingType"] = Json::Value(levelLoadedBeforeClosingType);
	result["levelLoadedBeforeClosingInfo"] = Json::Value(levelLoadedBeforeClosingInfo);
	return result;
}

auto GameSettings::fromJson(const Json::Value& json) -> GameSettings {
	return GameSettings{
		.levelLoadedBeforeClosingType = json.contains("levelLoadedBeforeClosingType") ? json.at("levelLoadedBeforeClosingType").string() : "",
		.levelLoadedBeforeClosingInfo = json.contains("levelLoadedBeforeClosingInfo") ? json.at("levelLoadedBeforeClosingInfo").string() : "",
	};
}

