#include <math/triangulate.hpp>

// https://www.geometrictools.com/Documentation/TriangulationByEarClipping.pdf
auto SimplePolygonTriangulator::operator()(Span<const Vec2> vertices) -> const std::vector<Triangle>& {
	if (vertices.size() < 3) {
		ASSERT_NOT_REACHED();
		result.clear();
		return result;
	}

	// Formula for signed area explained here https://gamemath.com/book/geomprims.html - 9.6.2 Area of a Triangle
	auto twiceTheSignedArea = 0.0f;
	auto f = [](Vec2 a, Vec2 b) {
		return (b.x - a.x) * (b.y + a.y);
	};
	twiceTheSignedArea += f(vertices[0], vertices[1]);
	for (usize i = 1; i < vertices.size() - 1; i++) {
		twiceTheSignedArea += f(vertices[i], vertices[i + 1]);
	}
	twiceTheSignedArea += f(vertices[vertices.size() - 1], vertices[0]);

	verts.clear();
	for (auto& vert : vertices)
		verts.push_back(vert);

	// If the signed area is negative then the vertices are counter-clockwise.
	if (const auto isCounterClockwise = twiceTheSignedArea < 0.0f)
		std::reverse(verts.begin(), verts.end());

	result.clear();
	usize iter = 0;
	while (verts.size() > 3) {
		if (iter > vertices.size())
			return result;
		iter++;
		for (usize i = 0; i < verts.size(); i++) {
			const auto leftIndex = i == 0 ? verts.size() - 1 : i - 1;
			const auto rightIndex = i == verts.size() - 1 ? 0 : i + 1;
			auto& left = verts[leftIndex];
			auto& right = verts[rightIndex];
			// The algorithm makes sure that the vertices are in clockwise order. When they are the then if you take some point and make a line connecting it's neighbours if the lie lies inside the shape then the determinant of the lines is positive and nagative is it lies outside the shape. For example look at an L shape. Connecting the 2 endpoints creates creates a line that lies outside the shape. Because the top part is the first vertex and the bottom part is the second this just forms a flipped pair of basis, which means that the determinant has to be nagative.
			const auto lineLiesOutsideOfTheShape = det(left - verts[i], right - verts[i]) < 0.0f;
			if (lineLiesOutsideOfTheShape)
				continue;

			Triangle triangle{ right, left, verts[i] };

			// Check if the line is a diagonal <=> doesn't intersect any other lines <=> the triangle doesn't contain any points (only in the case of a simple polygon).
			for (usize j = 0; j < verts.size(); j++) {
				if (j == i || j == leftIndex || j == rightIndex)
					continue;

				if (triangle.contains(verts[j]))
					goto isNotAnEar;
			}

			verts.erase(verts.begin() + i);
			result.push_back(triangle);
			break;

		isNotAnEar:
			continue;
		}
	}

	result.push_back(Triangle{ verts[0], verts[1], verts[2] });

	return result;
}