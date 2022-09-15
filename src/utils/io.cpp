#include <pch.hpp>
#include <io.h>
#include <utils/asserts.hpp>

#include <stdio.h>

auto vput(const char* format, va_list args) -> void {
	va_list argsCopy;
	va_copy(argsCopy, args);

	char buffer[1023];
	const auto bytesWritten = vsnprintf(buffer, sizeof(buffer), format, args);

	if (bytesWritten < sizeof(buffer)) {
		OutputDebugString(buffer);
		return;
	}

	ASSERT_NOT_REACHED();
	// TODO: Use frame allocator to allocate an array big enough;
}

auto put(const char* format, ...) -> void {
	va_list args;
	va_start(args, format);
	vput(format, args);
	va_end(args);
}

auto logInfoImpelementation(int, const char*, const char*, const char* format, ...) -> void {
	put("INFO: ");
	va_list args;
	va_start(args, format);
	vput(format, args);
	va_end(args);
	put("\n");
}

auto logWarningImpelementation(int line, const char* filename, const char* functionName, const char* format, ...) -> void {
	put("WARNING %s:%d in %s: ", filename, line, functionName);
	va_list args;
	va_start(args, format);
	vput(format, args);
	va_end(args);
	put("\n");
}

auto logFatalImpelementation(int line, const char* filename, const char* functionName, const char* format, ...) -> void {
	put("WARNING %s:%d in %s: ", filename, line, functionName);
	va_list args;
	va_start(args, format);
	vput(format, args);
	va_end(args);
	put("\n");
	DebugBreak();
	exit(EXIT_FAILURE);
}
