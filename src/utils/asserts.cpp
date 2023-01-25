#include <pch.hpp>
#include <utils/asserts.hpp>

auto assert_impl() -> void {
	__debugbreak();
}
