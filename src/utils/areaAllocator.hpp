#pragma once

#include <utils/int.hpp>

#include <string_view>

class AreaAllocator {
public:
	explicit AreaAllocator(usize maxSizeBytes);
	~AreaAllocator();
	auto allocAlligned(usize alignment, usize sizeBytes) -> void*;
	auto alloc(usize sizeBytes) -> void*;
	auto format(const char* format, ...) -> std::string_view;

	auto reset() -> void;

private:
	u8* const memory;
	u8* current;
	const usize maxSizeBytes;
	const usize pageSize;
};