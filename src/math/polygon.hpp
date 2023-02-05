#pragma once

#include <math/vec2.hpp>
#include <utils/span.hpp>

auto simplePolygonSignedArea(Span<const Vec2> verts) -> float;
auto simplePolygonArea(Span<const Vec2> verts) -> float;
auto simplePolygonIsClockwise(Span<const Vec2> verts) -> bool;

// It might not make sense to cut holes inside simplex polygons, because if the 2 shapes overlap it might be better to just subtract the holes shape from the outside shape.

// https://cs.stackexchange.com/questions/43758/turning-a-simple-polygon-with-holes-into-exterior-bounded-only
auto lineSegmentIntersectsSimplePolygon(Vec2 start, Vec2 end, Span<const Vec2> verts) -> bool;

// Can return zero area shapes.
// Can create self intersections.
auto polygonDouglassPeckerSimplify(Span<const Vec2> verts, float maxDistanceFromLine) -> std::vector<Vec2>;