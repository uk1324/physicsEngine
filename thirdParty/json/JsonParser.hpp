#pragma once

#include "JsonValue.hpp"

#include <string_view>

namespace Json
{
	class ParsingError : public JsonError {};
	Value parse(std::string_view text);
}