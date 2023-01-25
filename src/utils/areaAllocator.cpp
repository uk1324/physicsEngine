#include <pch.hpp>
#include <utils/areaAllocator.hpp>
#include <utils/io.hpp>
#include <winUtils.hpp>

#include <memory>

// page fault != access violation
static auto pageFaultExceptionFilter(DWORD exceptionCode) -> INT {
	if (exceptionCode == EXCEPTION_ACCESS_VIOLATION)
		return EXCEPTION_EXECUTE_HANDLER;
	return EXCEPTION_CONTINUE_EXECUTION;
};

AreaAllocator::AreaAllocator(usize maxSizeBytes) 
	: maxSizeBytes{ maxSizeBytes }
	// Because the pages only get commited when they are accessed if the allocation isn't too big you can call it with (MEM_RESERVE | MEM_COMMIT). Not sure if this would be a good option here.
	, memory{ reinterpret_cast<u8*>(VirtualAlloc(nullptr, maxSizeBytes, MEM_RESERVE, PAGE_READWRITE)) }
	, pageSize{ [] {
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		return sysInfo.dwPageSize;
	}() } {
	CHECK_WIN_ZERO(memory);
	CHECK_WIN_ZERO(VirtualAlloc(memory, pageSize, MEM_COMMIT, PAGE_READWRITE));
	__try {
		*(memory) = '\0';
	} __except (pageFaultExceptionFilter(GetExceptionCode())) {
		LOG_FATAL("failed to commit area page");
	}
	current = memory;
}

AreaAllocator::~AreaAllocator() {
	CHECK_WIN_BOOL(VirtualFree(memory, 0, MEM_RELEASE));
}

// Could fall back on malloc if the allocation fails.
auto AreaAllocator::allocAlligned(usize alignment, usize sizeBytes) -> void* {
	usize bytesLeft = (memory + maxSizeBytes) - current;
	void* curr = current;
	const auto aligned = reinterpret_cast<u8*>(std::align(alignment, sizeBytes, curr, bytesLeft));
	if (aligned == nullptr)
		LOG_FATAL("area allocation failed");

	auto pageNumber = [this](u8* address) { return (reinterpret_cast<usize>(address) - reinterpret_cast<usize>(memory)) / pageSize; };
	const auto currentPageNumber = pageNumber(current);
	const auto newPageNumber = pageNumber(aligned + sizeBytes);

	if (currentPageNumber == newPageNumber) {
		current = aligned + sizeBytes;
		return aligned;
	}

	CHECK_WIN_ZERO(VirtualAlloc(memory + (currentPageNumber + 1) * pageSize, (newPageNumber - currentPageNumber) * pageSize, MEM_COMMIT, PAGE_READWRITE));
	for (auto i = currentPageNumber + 1; i < newPageNumber; i++) {
		__try {
			// The pages only get commited if the memory is accessed.
			*(memory + pageSize * i) = '\0';
		} __except (pageFaultExceptionFilter(GetExceptionCode())) {
			LOG_FATAL("failed to commit area page");
		}
	}

	current = aligned + sizeBytes;
	return aligned;
}

auto AreaAllocator::alloc(usize sizeBytes) -> void* {
	return allocAlligned(16, sizeBytes);
}

auto AreaAllocator::format(const char* format, ...) -> std::string_view {
	va_list args;
	va_list argsCopy;
	va_start(args, format);
	va_copy(argsCopy, args);
	const auto requiredBufferSize = vsnprintf(nullptr, 0, format, args) + 1 /* null byte */;
	va_end(args);

	const auto buffer = reinterpret_cast<char*>(allocAlligned(1, requiredBufferSize));
	vsnprintf(buffer, requiredBufferSize, format, argsCopy);
	va_end(argsCopy);

	return std::string_view(buffer, requiredBufferSize);
}

auto AreaAllocator::reset() -> void {
	current = memory;
}