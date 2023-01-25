#include <demos/triangulationDemo.hpp>
#include <game/debug.hpp>
#include <engine/renderer.hpp>
#include <engine/input.hpp>
#include <math/mat2.hpp>
#include <random>

#include <imgui/imgui.h>

// If any concave shape can be represented using triangles. Then any concave shapes with holes can be created by cutting out triangles from the shape. To cut out a shape could look at the intersections of the lines (infinite lines) created from the sides of the triangle, that is trying to be cut out.

// If you were to just try to triangulate an O shape by making only making cut at the top and then running this algorithm there might be issues, because of vertices intersecting. It would be better to separate the shape into 2 shapes without holes and the run the algorithm on both.

// A different attempt at making a triangulation algorithm.
// Find the convex hull of the shape. Find the points which lie in the convex hull. Try to cut them off by trying to connect them with it's left and and next left right and next right neighbour. Only choose the cuts which don't self intersect. Not sure if this algorithm works. One big issue is the for many shapes most of the points lie inside the convex hull and not on it. For example a spiral. Finding the correct points to cut this way may take a lot of iterations.

// This algorithm 
// Try to connect a point p and it's neighbours into a triangle which lies inside the shape and doesn't intersect it.
// If found the remove the point p and do the next vertex and add the triangle(p, leftNeighbour, rightNeighbour) to the list.
// else try the next vertex.
// Do this until there is only a single triangle left.
// It turns out this is basically just the Ear clipping method for polygon triangulation.
static auto myTriangulationAlgorithm(const std::vector<Vec2>& points) -> std::optional<std::vector<Triangle>> {
	if (points.size() < 3)
		return std::nullopt;

	std::vector<LineSegment> lineSegments;

	auto calculateLineSegments = [&]() -> void {
		lineSegments.clear();
		if (points.size() >= 2) {
			lineSegments.push_back(LineSegment{ points[0], points[1] });
			for (usize i = 1; i < points.size() - 1; i++) {
				lineSegments.push_back(LineSegment{ points[i], points[i + 1] });
			}
			if (points.size() > 2) {
				lineSegments.push_back(LineSegment{ points[points.size() - 1], points[0] });
			}
		}
	};

	calculateLineSegments();
	for (const auto& a : lineSegments) {
		for (const auto& b : lineSegments) {
			if (&a == &b)
				continue;
			const auto [a0, a1] = a.getCorners();
			const auto [b0, b1] = b.getCorners();

			const auto intersection = a.intersection(b);
			if (intersection.has_value()) {
				const auto intersectionDoesntLieOnLineEndPoints = distance(*intersection, a0) > 0.001f && distance(*intersection, a1) > 0.001f && distance(*intersection, b0) > 0.001f && distance(*intersection, b1) > 0.001f;
				if (intersectionDoesntLieOnLineEndPoints) {
					// Self intersecting.
					return std::nullopt;
				}
			}
		}
	}

	auto aabb = Aabb::fromCorners(points[0], points[1]);
	for (usize i = 2; i < points.size(); i++) {
		aabb = aabb.extended(points[i]);
	}
	Debug::drawAabb(aabb);

	// Check if point is inside shape using the even-odd rule.
	auto isPointInsideShape = [&](Vec2 p) -> bool {
		// Start outside of the shape.
		auto current = Vec2{ aabb.min.x - 0.2f, (aabb.min.y + aabb.max.y) / 2.0f }; // Chosen arbitrarly
		// Ignore the segment that was hit previously.
		std::optional<LineSegment*> toIgnore;
		usize hitCount = 0;
		for (;;) {
			std::optional<Vec2> closestHitPoint;
			std::optional<LineSegment*> closestHitSegment;
			auto closestHitPointDistance = std::numeric_limits<float>::infinity();
			for (auto& segment : lineSegments) {
				if (&segment == toIgnore)
					continue;

				// raycastHit returns nullopt if the ray is parallel. So it won't detect if the rays are collinear. Which probably shouldn't be an issue, because if it is collinear it lines on the boundry of the shape anyway.
				const auto hit = segment.raycastHit(current, p);
				if (hit.has_value()) {
					if (closestHitPoint.has_value()) {
						if (const auto d = distance(*hit, current); d < closestHitPointDistance) {
							closestHitPointDistance = d;
							closestHitPoint = *hit;
							closestHitSegment = &segment;
						}
					} else {
						closestHitPoint = *hit;
						closestHitPointDistance = distance(*closestHitPoint, current);
						closestHitSegment = &segment;
					}
				}
			}

			if (!closestHitPoint.has_value()) {
				return hitCount % 2 == 1;
			}

			toIgnore = closestHitSegment;
			current = *closestHitPoint;
			hitCount++;

			if (distance(*closestHitPoint, p) < 0.001f) {
				return hitCount % 2 == 1;
			}
		}
	};

	std::vector<Triangle> triangles;
	std::vector<Vec2> vertices = points;

	auto iterations = 0;
	while (vertices.size() > 3) {
		if (iterations > 200)
			break;
		iterations++;

		calculateLineSegments();
		for (usize i = 0; i < vertices.size(); i++) {
			auto& left = i == 0 ? vertices[vertices.size() - 1] : vertices[i - 1];
			auto& right = i == vertices.size() - 1 ? vertices[0] : vertices[i + 1];

			const LineSegment line{ left, right };

			// Attempt to create a triangle from the points[i] and its left and right neighbours, by connecting the left and right neightbours, which creates a line opposite to points[i].
			for (const auto& segment : lineSegments) {
				const auto [start, end] = segment.getCorners();
				const auto intersection = line.raycastHit(start, end);
				if (intersection.has_value() && distance(*intersection, left) > 0.001f && distance(*intersection, right) > 0.001f && distance(*intersection, start) > 0.001f && distance(*intersection, end) > 0.001f) {
					// If the line intersects the shape then it isn't a correct triangulation. Epsilon checks to prevent finding false intersections on segment endpoints.
					goto tryNextVertex;
				}
				Debug::drawPoint(vertices[i], Vec3::GREEN);
			}

			// Example of a shape where the point lies inside the shape, but a triangulation would intersect the shape.
			// Draw a rectangle. Inside the rectangle draw an L, such that the bottom right part of the L touches the right part of the triangle.
			// Then remove the side which touches the left side of the rectangle. The resulting shape should look like the beginning of a spiral. 
			// The trying to draw a line connecting the endpoints of the L made from sides inside the shape you get a point which is inside the shape, but isn't the correct triangulation, because it intersects the shape.

			Debug::drawPoint((left + right) / 2.0f);
			// Check if the line lies inside the shape. This just checks if a single arbitrary points on the line lies inside the shape. Don't know if it is true that if a single point lines in the shape on a line which doesn't intersect the shape it means that the whole line lies inside the shape. If this is false then this algorithm is incorrect.
			if (isPointInsideShape((left + right) / 2.0f)) {
				Triangle tri{ left, right, vertices[i] };
				// Could try to detect triangles with small area, but if the case in which this is the only case isn't handled it might lead to infinite loops.
				// TODO: There might be a case, which creates 3 points which lie on the same line. In this case this needs to be detected by either checking area or something else and this point needs to be removed. If it isn't removed the it might never halt. Also check what effect does the intersects function returning false on collinear lines has on this. 
				triangles.push_back(tri);
				vertices.erase(vertices.begin() + i);
				break;
			}

			// If the point and it's neighbours can't be turned into a triangle then try the next point. There might be some cases in which the algorithm doesn't terminate. To prove that the algorithm does teminate, you would need to prove that for any closed, non self-intersecting concave polygon there is a wave to connect a points and its neighbours into a triangle, which doesn't self intersect the shape and lies inside the shape. This is true for convex shapes, because from the definition any line from a vertex to vertex lies inside the shape. For concave shapes it intuitivelly makes sense, because for there to be a concativty there needs to be some part which "is convex" or "concavities are created from convex parts".
			// It turns out this is called the Two ears theorem.
			tryNextVertex:
			continue;
		}
	}
	triangles.push_back(Triangle{ vertices[0], vertices[1], vertices[2] } );
	return triangles;
}

auto TriangulationDemo::update() -> void {
	Camera camera;

	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		points.push_back(camera.cursorPos());
	}


	if (points.size() >= 3 && showTriangulation) {
		const auto oldSize = triangles.size();
		triangles = triangulator(points);
		if (triangles.size() != oldSize) {
			j = static_cast<int>(triangles.size());
		}
	}

	Debug::drawLines(points);
	Debug::drawSimplePolygon(points);


	if (Input::isKeyDown(Keycode::D)) {
		showTriangulation = !showTriangulation;
	}

	if (showTriangulation) {
		for (int k = 0; k < j; k++) {
			const auto& triangle = triangles[k];
			Debug::drawLine(triangle.v[0], triangle.v[1], Vec3::RED);
			Debug::drawLine(triangle.v[1], triangle.v[2], Vec3::RED);
			Debug::drawLine(triangle.v[2], triangle.v[0], Vec3::RED);
		}
	}

	ImGui::SliderInt("vertex do highlight", &i, 0, static_cast<int>(points.size() - 1));
	ImGui::SliderInt("triangle count to display", &j, 0, static_cast<int>(triangles.size()));
	if (points.size() != 0) {
		Debug::drawPoint(points[i]);
	}
	if (ImGui::Button("shift points")) {
		std::rotate(points.begin(), points.begin() + 1, points.end());
	}
	Renderer::update(camera);
}