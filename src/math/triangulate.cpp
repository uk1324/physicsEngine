#include <math/triangulate.hpp>


//static float FindPolygonArea(const std::vector<Vec2>& vertices)
//{
//    float totalArea = 0.0f;
//
//    for (int i = 0; i < vertices.size(); i++)
//    {
//        auto a = vertices[i];
//        auto b = vertices[(i + 1) % vertices.size()];
//
//        float dy = (a.y + b.y) / 2.0f;
//        float dx = b.x - a.x;
//
//        float area = dy * dx;
//        totalArea += area;
//    }
//
//    return abs(totalArea);
//}
//
//static bool Triangulate(std::vector<Vec2> vertices, std::vector<int>& triangles)
//{
//    //triangles = null;
//    //errorMessage = string.Empty;
//
//
//    if (vertices.size() < 3)
//    {
//        ASSERT_NOT_REACHED();
//        return false;
//    }
//
//    if (vertices.size() > 1024)
//    {
//        //errorMessage = "The max vertex list length is 1024";
//        ASSERT_NOT_REACHED();
//        return false;
//    }
//
//    //if (!PolygonHelper.IsSimplePolygon(vertices))
//    //{
//    //    errorMessage = "The vertex list does not define a simple polygon.";
//    //    return false;
//    //}
//
//    //if(PolygonHelper.ContainsColinearEdges(vertices))
//    //{
//    //    errorMessage = "The vertex list contains colinear edges.";
//    //    return false;
//    //}
//
//    //PolygonHelper.ComputePolygonArea(vertices, out float area, out WindingOrder windingOrder);
//
//    //if(windingOrder is WindingOrder.Invalid)
//    //{
//    //    errorMessage = "The vertices list does not contain a valid polygon.";
//    //    return false;
//    //}
//
//    //if(windingOrder is WindingOrder.CounterClockwise)
//    //{
//    //    Array.Reverse(vertices);
//    //}
//
//    std::vector<int> indexList;
//    for (int i = 0; i < vertices.size(); i++)
//    {
//        indexList.push_back(i);
//    }
//
//    int totalTriangleCount = vertices.size() - 2;
//    int totalTriangleIndexCount = totalTriangleCount * 3;
//
//    //triangles = new int[];
//    triangles.resize(totalTriangleIndexCount, 0);
//    int triangleIndexCount = 0;
//
//    auto GetItem = [](std::vector<int>& v, int i) -> int {
//        if (i < 0) {
//            i = v.size() - 1;
//        } else if (i >= v.size()) {
//            i = 0;
//        }
//        return v[i];
//    };
//
//    int i = 0;
//    while (indexList.size() > 3)
//    {
//        if (i > vertices.size() * 4)
//            return false;
//        i++;
//        /*if (indexList.size() < 68) {
//            return true;
//        }*/
//        for (int i = 0; i < indexList.size(); i++)
//        {
//            int a = indexList[i];
//            int b = GetItem(indexList, i - 1);
//            int c = GetItem(indexList, i + 1);
//
//            auto va = vertices[a];
//            auto vb = vertices[b];
//            auto vc = vertices[c];
//
//            auto va_to_vb = vb - va;
//            auto va_to_vc = vc - va;
//
//            // Is ear test vertex convex?
//            if (cross(va_to_vb, va_to_vc) < 0.0f)
//            {
//                continue;
//            }
//
//            bool isEar = true;
//
//            // Does test ear contain any polygon vertices?
//            for (int j = 0; j < vertices.size(); j++)
//            {
//                if (j == a || j == b || j == c)
//                {
//                    continue;
//                }
//
//                auto p = vertices[j];
//
//                auto IsPointInTriangle = [](Vec2 p, Vec2 a, Vec2 b, Vec2 c)
//                {
//                    Vec2 ab = b - a;
//                    Vec2 bc = c - b;
//                    Vec2 ca = a - c;
//
//                    Vec2 ap = p - a;
//                    Vec2 bp = p - b;
//                    Vec2 cp = p - c;
//
//                    float cross1 = cross(ab, ap);
//                    float cross2 = cross(bc, bp);
//                    float cross3 = cross(ca, cp);
//
//                    if (cross1 > 0.0f || cross2 > 0.0f || cross3 > 0.0f)
//                    {
//                        return false;
//                    }
//
//                    return true;
//                };
//
//                //Triangle tri{ vb, va, vc };
//                if (IsPointInTriangle(p, vb, va, vc))
//                    //if (tri.contains(p))
//                {
//                    isEar = false;
//                    break;
//                }
//            }
//
//            if (isEar)
//            {
//                triangles[triangleIndexCount++] = b;
//                triangles[triangleIndexCount++] = a;
//                triangles[triangleIndexCount++] = c;
//
//                /*indexList.RemoveAt(i);*/
//                indexList.erase(indexList.begin() + i);
//                break;
//            }
//        }
//    }
//
//    triangles[triangleIndexCount++] = indexList[0];
//    triangles[triangleIndexCount++] = indexList[1];
//    triangles[triangleIndexCount++] = indexList[2];
//
//    return true;
//}
//auto SimplePolygonTriangulator::operator()(Span<const Vec2> vertices) -> const std::vector<Triangle>& {
//    std::vector<int> triangles;
//    std::vector<Vec2> vs;
//    for (const auto& a : vertices) {
//        vs.push_back(a);
//    }
//    Triangulate(vs, triangles);
//    result.clear();
//    for (int i = 0; i < triangles.size(); i += 3) {
//        result.push_back(Triangle{ vertices[triangles[i]], vertices[triangles[i + 1]], vertices[triangles[i + 2]] });
//    }
//    return result;
//}


// https://www.geometrictools.com/Documentation/TriangulationByEarClipping.pdf
auto SimplePolygonTriangulator::operator()(Span<const Vec2> vertices) -> const std::vector<Triangle>& {
	if (vertices.size() < 3) {
		ASSERT_NOT_REACHED();
		result.clear();
		return result;
	}

	// 6Formula for signed area explained here https://gamemath.com/book/geomprims.html - 9.6.2 Area of a Triangle
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
		if (iter > vertices.size() * 15)
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