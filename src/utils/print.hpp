#pragma once

#include <ostream>
#include <utils/asserts.hpp>

auto printToStream(std::ostream& os, const char* format) -> void {
	os << format;
}

template<typename T, typename ...Args>
auto printToStream(std::ostream& os, const char* format, const T& arg, const Args&... args) -> void {
	int current = 0;
	while (format[current] != '\0') {
		if (format[current] == '%') {
			current++;
			// This is safe because the next one is either a valid character or the string end '\0'.
			if (format[current] == '%') {
				current++;
			} else {
				os << arg;
				printToStream(os, format + current, args...);
				return;
			}
		}
		os << format[current];
		current++;
	}
	// A call with a correct format should always finish at the overload with no Args. 
	ASSERT_NOT_REACHED();
}

template<typename ...Args>
auto printToStream(std::ostream& os, const char* format, const Args&... args) -> void {
	return printToStream(os, format, args...);
}