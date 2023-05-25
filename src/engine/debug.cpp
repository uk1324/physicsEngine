#include <engine/debug.hpp>
#include <math/mat2.hpp>
#include <engine/frameAllocator.hpp>
#include <../debugger_tool/src/client.hpp>

std::optional<DebuggerClient> client;

auto Debug::update() -> void {
	// Istead of using an optional could have a check isConnected which would check if the handle is valid. This would allow the server to be closed and reopened.
	if (!client.has_value()) {
		client = DebuggerClient::connect();
	}
	lines.clear();
	circles.clear();
	points.clear();
	circleColliders.clear();
	hollowCircles.clear();
	parabolas.clear();
	triangles.clear();
	text.clear();
}

auto Debug::drawLine(Vec2 start, Vec2 end, const Vec3& color) -> void {
	lines.push_back({ start, end, color });
}

auto Debug::drawLineSegment(const LineSegment& lineSegment, const Vec3& color) -> void {
	const auto corners = lineSegment.getCorners();
	Debug::drawLine(corners[0], corners[1], color);
}

auto Debug::drawRay(Vec2 start, Vec2 ray, const Vec3& color) -> void {
	lines.push_back({ start, start + ray, color });
}

auto Debug::drawCircle(Vec2 pos, float radius, const Vec3& color) -> void {
	circles.push_back({ pos, radius, color });
}

auto Debug::drawHollowCircle(Vec2 pos, float radius, const Vec3& color) -> void {
	hollowCircles.push_back({ pos, radius, color });
}

auto Debug::drawCircleCollider(Vec2 pos, float radius, float orientation, const Vec3& color) -> void {
	circleColliders.push_back({ { pos, radius, color }, orientation });
}

auto Debug::drawPoint(Vec2 pos, const Vec3& color) -> void {
	points.push_back({ pos, 0.01f, color });
}

auto Debug::drawLines(Span<const Vec2> vertices, const Vec3& color) -> void {
	if (vertices.size() < 2)
		return;

	drawLine(vertices[0], vertices[1], color);

	if (vertices.size() == 2)
		return;

	for (usize i = 1; i < vertices.size() - 1; i++)
		drawLine(vertices[i], vertices[i + 1], color);
	drawLine(vertices.back(), vertices[0], color);
}

auto Debug::drawAabb(const Aabb& aabb, const Vec3& color) -> void {
	const auto v1 = Vec2{ aabb.max.x, aabb.min.y };
	const auto v3 = Vec2{ aabb.min.x, aabb.max.y };
	Debug::drawLine(aabb.min, v1, color);
	Debug::drawLine(v1, aabb.max, color);
	Debug::drawLine(aabb.max, v3, color);
	Debug::drawLine(v3, aabb.min, color);
}

auto Debug::drawParabola(float a, Vec2 pos, const Vec3& color) -> void {
	parabolas.push_back({ a, pos, color });
}

auto Debug::drawBox(Vec2 pos, float orientation, Vec2 size, const Vec3& color) -> void {
	const auto rotate = Mat2::rotate(orientation);
	// @Performance: Could just use the basis from the rotate matrix. Or even better precompute the matrix because it is used in a lot of places.
	const auto edgeX = Vec2{ size.x, 0.0f } * rotate;
	const auto edgeY = Vec2{ 0.0f, size.y } * rotate;
	const auto vertex1 = (size / 2.0f) * rotate + pos;
	const auto vertex2 = vertex1 - edgeX;
	const auto vertex3 = vertex2 - edgeY;
	const auto vertex4 = vertex3 + edgeX;
	Debug::drawLine(vertex1, vertex2, color);
	Debug::drawLine(vertex2, vertex3, color);
	Debug::drawLine(vertex3, vertex4, color);
	Debug::drawLine(vertex4, vertex1, color);
}

auto Debug::drawCollider(const Collider& collider, Vec2 pos, float orientation, const Vec3& color) -> void {
	if (const auto box = std::get_if<BoxCollider>(&collider)) Debug::drawBox(pos, orientation, box->size, color);
	else if (const auto circle = std::get_if<CircleCollider>(&collider)) Debug::drawCircleCollider(pos, circle->radius, orientation, color);
	else if (const auto poly = std::get_if<ConvexPolygon>(&collider)) {
		const Transform transform{ pos, orientation };
		for (i32 i = 0; i < poly->verts.size(); i++) {
			auto next = i + 1;
			if (next >= poly->verts.size()) {
				next = 0;
			}
			Debug::drawLine(poly->verts[i] * transform, poly->verts[next] * transform, color);
		}
	}
	else ASSERT_NOT_REACHED();
}

auto Debug::drawSimplePolygon(Span<const Vec2> vertices, const Vec3& color) -> void {
	if (vertices.size() < 3)
		return;

	const auto& tris = triangulate(vertices);
	for (const auto& triangle : tris) {
		triangles.push_back(Triangle{ triangle, color });
	}
}

auto Debug::drawStr(Vec2 pos, const char* txt, const Vec3& color, float height) -> void {
	auto length = strlen(txt) + 1;
	auto t = frameAllocator.alloc(length);
	memcpy(t, txt, length);
	Debug::text.push_back(Text{ pos, reinterpret_cast<char*>(t), color, height });
}

auto Debug::debugImage(const ImageRgba* img) -> void {
	if (!client.has_value())
		return;
	client->send(RefreshArray2dGridMessage::image32Grid(img->data(), Vec2T<i32>{ img->size() }));
}

auto Debug::debugU8Array2d(u8* data, Vec2T<i64> size, u8 min, u8 max, bool posXGoingRight, bool posYGoingUp) -> void {
	if (!client.has_value())
		return;
	client->send(RefreshArray2dGridMessage::intGrid(data, Vec2T<i32>{ size }, Array2dType::U8, min, max, posXGoingRight, posYGoingUp));
}

auto Debug::debugF32Array2d(float* data, Vec2T<i64> size, float min, float max, bool posXGoingRight, bool posYGoingUp) -> void {
	if (!client.has_value())
		return;
	client->send(RefreshArray2dGridMessage::floatGrid(data, Vec2T<i32>{ size }, Array2dType::F32, min, max, posXGoingRight, posYGoingUp));
}

std::vector<Debug::Line> Debug::lines;
std::vector<Debug::Circle> Debug::circles;
std::vector<Debug::Point> Debug::points;
std::vector<Debug::OrientedCircle> Debug::circleColliders;
std::vector<Debug::Circle> Debug::hollowCircles;
std::vector<Debug::Parabola> Debug::parabolas;
std::vector<Debug::Triangle> Debug::triangles;
std::vector<Debug::Text> Debug::text;