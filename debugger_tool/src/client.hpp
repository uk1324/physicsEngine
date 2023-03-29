#pragma once

#include <utils/int.hpp>
#include <optional>

struct DebuggerClient {
	static auto connect() -> std::optional<DebuggerClient>;
	auto send(const void* message, usize messageByteSize) -> void;
	template<typename T>
	auto send(const T& message) -> void;

	DebuggerClient(const DebuggerClient&) = delete;
	auto operator= (const DebuggerClient&) -> DebuggerClient& = delete;
	DebuggerClient(DebuggerClient&& other) noexcept;
	auto operator= (DebuggerClient&& other) noexcept -> DebuggerClient& ;
	~DebuggerClient();

	void* handle;
private:
	DebuggerClient(void* handle);
};

template<typename T>
auto DebuggerClient::send(const T& message) -> void {
	send(&message, sizeof(message));
}
