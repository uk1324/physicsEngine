#pragma once

#include <sstream>

// The issue with std::stringstream is that it creates a copy of the string each time you access it. This class allows the user to directly access the internal std::string.
struct StringStream : std::ostream {
	struct StringStreamBuf : public std::stringbuf {
		auto overflow(int_type c) -> int_type override;
		std::string buffer;
	};
	StringStream();

	StringStreamBuf buffer;

	auto string() -> std::string&;
	auto string() const -> const std::string&;
};