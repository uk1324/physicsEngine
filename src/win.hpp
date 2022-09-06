#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>

#include <asserts.hpp>

auto getLastErrorString() -> std::string;

#define CHECK_WIN_ZERO(expression) \
	do { \
		const auto result = expression; \
		if (result == 0) { \
			\
			const auto result = MessageBox(nullptr, getLastErrorString().c_str(), nullptr, MB_ICONEXCLAMATION); \
			ASSERT(result != 0); \
			DebugBreak(); \
			exit(EXIT_FAILURE); \
		} \
	} while (false);

#define CHECK_WIN_BOOL(expression) CHECK_WIN_ZERO(expression)
#define CHECK_WIN_NULL(expression) CHECK_WIN_ZERO(expression)