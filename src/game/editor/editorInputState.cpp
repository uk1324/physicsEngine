#include <game/editor/editorInputState.hpp>

auto EditorGuiState::updateBeforeOpeningGui() -> void {
	if (updatedNewSaveDataLastCall) {
		oldSavedDataIndex = (oldSavedDataIndex + 1) % 2;
	}
}

auto EditorGuiState::placeToSaveDataAfterNewChange() -> u8* {
	updatedNewSaveDataLastCall = true;
	return savedInputData[(oldSavedDataIndex + 1) % 2];
}

auto EditorGuiState::oldSavedData() -> u8* {
	return savedInputData[oldSavedDataIndex];
}
