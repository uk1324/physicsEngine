#pragma once

[[noreturn]] auto onWinError(long errorCode, const char* filename, int line) -> void;

#define CHECK_WIN_ZERO(expr) \
	do { \
		if ((expr) == 0) { \
			onWinError(GetLastError(), __FILE__, __LINE__); \
		} \
	} while (false)

#define CHECK_WIN_BOOL(expr) CHECK_WIN_ZERO(expr)
#define CHECK_WIN_NULL(expr) CHECK_WIN_ZERO(expr)

#define CHECK_WIN_HRESULT(expr) \
	do { \
		if (FAILED(expr)) { \
			onWinError(expr, __FILE__, __LINE__); \
		} \
	} while (false)

#define CHECK_WIN_HANDLE(expr) \
	do { \
		if (expr == INVALID_HANDLE_VALUE) { \
			onWinError(GetLastError(), __FILE__, __LINE__); \
		} \
	} while (false)