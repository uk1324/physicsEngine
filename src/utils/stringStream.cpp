#include <utils/stringStream.hpp>

StringStream::StringStream() :
	std::ostream{ &buffer } {}

auto StringStream::StringStreamBuf::overflow(int_type c) -> int_type {
	buffer += static_cast<char>(c);
	// Not sure what should be returned here. https://en.cppreference.com/w/cpp/io/basic_streambuf/overflow
	return 0;
}

auto StringStream::string() -> std::string& {
	return buffer.buffer;
}
auto StringStream::string() const -> const std::string& {
	return buffer.buffer;
}
