#pragma once

#include <math/triangle.hpp>
#include <utils/span.hpp>

#include <vector>

struct SimplePolygonTriangulator {
	/*
	There can be numerical issues for example when the data is to aligned to a grid (for example vertices from marching squares on a pixel image. This can create cases when the intermediate triangulated polygon's point lies on one of the lines. Example of this can be seen in assets/example1.jpg. The issuse in the image was caused by the function checking if a point is inside a triangle not considering points on the boundary as inside. The issue is that doing the correct point in triangle test can also cause a lot of issues. When running the Bad Apple test ("Mesh simplification" commit) with the test that considers points on the boundary as inside it creates a lot of glitches.
	One way to deal with this issue would be to actually perform the intersection test instead of just checking if there are any points inside to find if there are intersections, but there can be numerical issues with this approach as well as described in the line segment vs polygon intersection test in math/polygon.hpp.
	Another numerical issue which happens even with the correct point in triangle test is zero area triangles. One example can be seen in assets/example2.jpg. The diagonal going from the bottom left to the top right actually has a point in the middle so this creates a zero area triangle (bottom left, top right, middle). 
	TODO: To fix this could probably skip triangles that are zero area. This might cause issues with some things not getting fully triangulated because the (iter > vertices.size()) check might return from the function before the shape got fully triangulated. I prefer to keep this check here so even in the cases when invalid data is given the algorithm always halts. There might even be cases when intermediate steps create self intersecting or other invalid data.
	*/
	// The resulting triangles are clockwise.
	auto operator()(Span<const Vec2> vertices) -> const std::vector<Triangle>&;

private:
	std::vector<Vec2> verts;
	std::vector<Triangle> result;
};