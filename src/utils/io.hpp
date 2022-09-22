#pragma once

#include <varargs.h>
#include <sstream>

auto vput(const char* format, va_list args) -> void;
auto put(const char* format, ...) -> void;

auto logInfoImpelementation(int line, const char* filename, const char* functionName, const char* format, ...) -> void;
auto logWarningImpelementation(int line, const char* filename, const char* functionName, const char* format, ...) -> void;
[[noreturn]] auto logFatalImpelementation(int line, const char* filename, const char* functionName, const char* format, ...) -> void;

#define LOG_INFO(format, ...) logInfoImpelementation(__LINE__, __FILE__, __FUNCSIG__, format, __VA_ARGS__)
#define LOG_WARNING(format, ...) logWarningImpelementation(__LINE__, __FILE__, __FUNCSIG__, format, __VA_ARGS__)
#define LOG_FATAL(format, ...) logFatalImpelementation(__LINE__, __FILE__, __FUNCSIG__, format, __VA_ARGS__)

template<typename T>
auto debugImplementation(T&& value, const char* expressionString, int line, const char* function) -> T {
	std::stringstream stream;
	stream << value;
	put("line %d in %s(): %s = %s\n", line, function, expressionString, stream.str().c_str());
	return value;
}

#define dbg(value) debugImplementation(value, #value, __LINE__, __func__)