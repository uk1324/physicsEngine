#include <win.hpp>

auto getLastErrorString() -> std::string
{
	const auto errorCode = GetLastError();

	ASSERT(errorCode != 0);

	char* message;
	const auto messageLength = FormatMessage(
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
