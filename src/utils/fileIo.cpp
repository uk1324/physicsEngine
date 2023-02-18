#include <utils/fileIo.hpp>

#include <sstream>

auto readFileToString(const std::ifstream& file) -> std::string {
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

auto readFileToString(const char* path) -> std::string {
	std::ifstream file{ path };
	return readFileToString(file);
}
