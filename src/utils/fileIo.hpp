#pragma once

#include <string>
#include <fstream>

auto readFileToString(const std::ifstream& file) -> std::string;
auto readFileToString(const char* path) -> std::string;