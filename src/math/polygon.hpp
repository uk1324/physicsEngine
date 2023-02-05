#pragma once

#include <math/vec2.hpp>
#include <utils/span.hpp>

auto simplePolygonSignedArea(Span<const Vec2> verts) -> float;
auto simplePolygonArea(Span<const Vec2> verts) -> float;
auto simplePolygonIsClockwise(Span<const Vec2> verts) -> bool;