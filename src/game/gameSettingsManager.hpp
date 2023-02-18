#pragma once

#include <game/gameSettingsData.hpp>

struct GameSettingsManager : public GameSettings {
	struct RaiiSaver {
		auto operator->() -> GameSettings*;
		~RaiiSaver();
	};
	// close button, alt+f4, alt+ctrl+del calls the destructor.
	// stop debugging doesn't call. I though it might just not be monitoring the program but it also doesn't write to file which means it doesn't get called.
	// segfault doesn't call.
	// So in general it seems that crashing the program doesn't call. And if the program is terminated by some other program that lunched it then it also doesn't get called.
	// So saving on each modification seems better than saving only at the end.

	GameSettingsManager();
	auto saveToFile() const -> void;
	[[nodiscard]] auto saveAtScopeEnd() const -> RaiiSaver;
	// Sometimes it might be easier to use saveToFile directly instead of using RaiiSaver. For example when using ImGui inputs to modift the values. Then if a RaiiSaver was used it would write to the file every frame even if nothing was modified. Could implement a system that check if the state was modified, but that would need to make a copy of the file and each type would also need to be comparable, which isn't that big of an issues because of operator=() = default. An approximate solution would be to store a hash of the settings.

private:
	const std::string settingsPath;
};

extern GameSettingsManager gameSettings;