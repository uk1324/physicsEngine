#pragma once

#include "JsonValue.hpp"

#include <string_view>

namespace Json
{
	class ParsingError : public std::exception {};
	Value parse(std::string_view text);
}