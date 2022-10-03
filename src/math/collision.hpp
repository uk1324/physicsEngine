#pragma once

#include <math/line.hpp>

#include <optional>

auto rayVsLine(Vec2 v, const Line& line) -> std::optional<float>;