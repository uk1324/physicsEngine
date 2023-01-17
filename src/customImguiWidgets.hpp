#pragma once

#include <utils/imageRgba.hpp>

namespace ImGui {

static constexpr auto MIN_FILE_SELECT_STRING_LENGTH = 256;

auto fileSelect(const char* label, char* outPath, int maxPathStringLength, const char* filterString = nullptr, const char* initialDirectory = nullptr, bool displayText = true) -> bool;
auto imageFileSelect(const char* label, const char* initialDirectory = nullptr) -> std::optional<ImageRgba>;
auto imageSaveFileSelect(const ImageRgba& image, const char* label, const char* initialDirectory = nullptr) -> void;
auto inputAngle(const char* label, float* angle) -> bool;
auto sliderAngle(const char* label, float* angle) -> bool;
}