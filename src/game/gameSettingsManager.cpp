#include <game/gameSettingsManager.hpp>
#include <utils/fileIo.hpp>
#include <json/Json.hpp>

#include <filesystem>

GameSettingsManager gameSettings;

GameSettingsManager::GameSettingsManager()
	// Creating the path at runtime because some functions change the current working directory. For example GetOpenFileNameA.
	// Initializing it here to ensure the correct order of static intialization.
	: settingsPath{ (std::filesystem::current_path() / "settings.json").string() }{
	const auto fileStr = readFileToString(settingsPath.data());
	try {
		const auto fileJson = Json::parse(fileStr);
		GameSettings::operator=(GameSettings::fromJson(fileJson));
	} catch (const Json::JsonError&) {
		// else it just uses the default initialization.
	}
}

auto GameSettingsManager::saveToFile() const -> void {
	std::ofstream file{ settingsPath };
	Json::prettyPrint(file, GameSettings::toJson());
}

auto GameSettingsManager::saveAtScopeEnd() const -> RaiiSaver {
	return RaiiSaver{};
}

auto GameSettingsManager::RaiiSaver::operator->() -> GameSettings* {
	return &gameSettings;
}

GameSettingsManager::RaiiSaver::~RaiiSaver() {
	gameSettings.saveToFile();
}
