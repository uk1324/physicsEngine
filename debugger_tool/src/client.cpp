#include <client.hpp>
#include <utils/asserts.hpp>
#include <winUtils.hpp>
#include <messages.hpp>

#include <windows.h>

DebuggerClient::DebuggerClient(DebuggerClient&& other) noexcept 
	: handle{ other.handle } {
	other.handle = INVALID_HANDLE_VALUE;
}

auto DebuggerClient::operator=(DebuggerClient&& other) noexcept -> DebuggerClient& {
	handle = other.handle;
	other.handle = INVALID_HANDLE_VALUE;
	return *this;
}

auto DebuggerClient::connect() -> std::optional<DebuggerClient> {
	const auto pipeFile = CreateFileA("\\\\127.0.0.1\\pipe\\debugger", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (pipeFile == INVALID_HANDLE_VALUE) {
		return std::nullopt;
	}
	DebuggerClient client{ pipeFile };

	ConnectMessage connect{ .processId = GetCurrentProcessId() };
	client.send(connect);
	return client;
}

auto DebuggerClient::send(const void* message, usize messageByteSize) -> void {
	DWORD bytesWritten;
	CHECK_WIN_BOOL(WriteFile(handle, message, static_cast<DWORD>(messageByteSize), &bytesWritten, nullptr));
	ASSERT(bytesWritten == messageByteSize);
}

DebuggerClient::~DebuggerClient() {
	CloseHandle(handle);
}

DebuggerClient::DebuggerClient(void* handle)
	: handle{ handle } {}
