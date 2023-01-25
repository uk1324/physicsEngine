#include <pch.hpp>
#include <commdlg.h>
#include <customImguiWidgets.hpp>
#include <engine/window.hpp>
#include <engine/frameAllocator.hpp>
#include <math/utils.hpp>

#include <imgui/imgui.h>

using namespace ImGui;

auto ImGui::fileSelect(const char* label, char* outPath, int maxPathStringLength, const char* filterString, const char* initialDirectory, bool displayText) -> bool {
	ASSERT(maxPathStringLength >= MIN_FILE_SELECT_STRING_LENGTH);
	OPENFILENAMEA openFile{
		.lStructSize = sizeof(OPENFILENAMEA),
		.hwndOwner = reinterpret_cast<HWND>(Window::hWnd()),
		.lpstrFilter = filterString,
		.nFilterIndex = 1,
		.lpstrFile = outPath,
		.nMaxFile = static_cast<DWORD>(maxPathStringLength),
		.lpstrFileTitle = nullptr,
		.nMaxFileTitle = 0,
		.lpstrInitialDir = initialDirectory,
		.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER
	};

	if (Button(label)) {
		// The function requires the buffer to be the null string. 
		// To keep the value when the select is cancalled could allocate the memory for the result using the frame allocator. The function doesn't say anything about what happens to lpstrFile when it returns false so saving the first character replacing it with zero and then restoring later if false it returned might not work.
		outPath[0] = '\0';
		if (GetOpenFileNameA(&openFile)) {
			return true;
		} else {
			ASSERT(CommDlgExtendedError() == 0);
		}
	} 
	if (displayText) {
		SameLine();
		if (strlen(outPath) == 0) {
			Text("No file chosen.");
		} else {
			Text("%s", outPath);
		}
	}

	return false;
}



auto ImGui::imageFileSelect(const char* label, const char* initialDirectory) -> std::optional<ImageRgba> {
	char path[MIN_FILE_SELECT_STRING_LENGTH] = "";
	if (!fileSelect(label, path, sizeof(path), "image\0*.png;*.jpg;*.bmp\0", initialDirectory, false)) {
		return std::nullopt;
	}
	return ImageRgba::fromFile(path);
}

auto ImGui::imageSaveFileSelect(const ImageRgba& image, const char* label, const char* initialDirectory) -> void {
	char path[MIN_FILE_SELECT_STRING_LENGTH] = "";
	OPENFILENAMEA openFile{
		.lStructSize = sizeof(OPENFILENAMEA),
		.hwndOwner = reinterpret_cast<HWND>(Window::hWnd()),
		.lpstrFilter = "(*.png)\0*.png\0",
		.nFilterIndex = 1,
		.lpstrFile = path,
		.nMaxFile = sizeof(path),
		.lpstrFileTitle = nullptr,
		.nMaxFileTitle = 0,
		.lpstrInitialDir = initialDirectory,
		.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER,
	};


	if (!Button(label))
		return;

	if (!GetSaveFileNameA(&openFile)) {
		ASSERT(CommDlgExtendedError() == 0);
		return;
	}

	// .lpstrDefExt = "png" should set the default extension, but it doesn't work.
	image.saveToFile(frameAllocator.format("%s.png", path).data());
}

auto ImGui::inputAngle(const char* label, float* angle) -> bool {
	float angleDeg = radToDeg(*angle);
	const auto changed = InputFloat(label, &angleDeg);
	*angle = degToRad(angleDeg);
	return changed;
}

auto ImGui::sliderAngle(const char* label, float* angle) -> bool{
	float angleDeg = radToDeg(*angle);
	const auto changed = SliderFloat(label, &angleDeg, -180, 180);
	*angle = degToRad(angleDeg);
	return changed;
}
