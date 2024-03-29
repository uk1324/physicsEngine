#include <pch.hpp>
#include <winUtils.hpp>
#include <utils/asserts.hpp>

#include <string>

static auto getErrorMessage(long errorCode) -> std::string {
	ASSERT(errorCode != 0);

	char* message;
	const auto messageLength = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&message),
		0,
		nullptr
	);

	// TODO: Maybe use frame allocator.

	if (messageLength == 0) {
		return "Invalid or unknown error code";
	}

	std::string messageString{ message };

	const auto result = LocalFree(message);
	ASSERT(result == nullptr);

	return messageString;
}

auto onWinError(long errorCode, const char*, int) -> void {
	const auto result = MessageBoxA(nullptr, getErrorMessage(errorCode).c_str(), nullptr, MB_ICONEXCLAMATION);
	ASSERT(result != 0);
	DebugBreak();
	exit(EXIT_FAILURE);
}