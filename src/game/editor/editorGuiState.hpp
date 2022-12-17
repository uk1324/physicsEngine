#pragma once

#include <utils/int.hpp>

struct EditorGuiState {
	bool inputing = false;

	auto updateBeforeOpeningGui() -> void;
	auto placeToSaveDataAfterNewChange() -> u8*;
	auto oldSavedData() -> u8*;

private:
	bool updatedNewSaveDataLastCall = false;
	i32 oldSavedDataIndex = 0;
	u8 savedInputData[256][2];
};