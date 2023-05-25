#pragma once

#include <math/aabb.hpp>
#include <math/vec2.hpp>
#include <math/vec3.hpp>
#include <math/triangulate.hpp>
#include <game/collider.hpp>
#include <utils/span.hpp>
#include <utils/imageRgba.hpp>

#include <vector>
#include <sstream>

namespace Debug {
	auto update() -> void;

	static constexpr auto DEFAULT_COLOR = Vec3{ 1.0f, 1.0f, 1.0f };

	auto drawLine(Vec2 start, Vec2 end, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawLineSegment(const LineSegment& lineSegment, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawRay(Vec2 start, Vec2 ray, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawCircle(Vec2 pos, float radius = 0.01f, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawHollowCircle(Vec2 pos, float radius = 0.01f, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawCircleCollider(Vec2 pos, float radius = 0.01f, float orientation = 0.0f, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawPoint(Vec2 pos, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawLines(Span<const Vec2> vertices, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawAabb(const Aabb& aabb, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawParabola(float a, Vec2 pos, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawBox(Vec2 pos, float orientation, Vec2 size, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawCollider(const Collider& collider, Vec2 pos, float orientation, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawSimplePolygon(Span<const Vec2> vertices, const Vec3& color = DEFAULT_COLOR) -> void;
	auto drawStr(Vec2 pos, const char* text, const Vec3& color = DEFAULT_COLOR, float height = 0.1f) -> void;
	template<typename T>
	auto drawText(Vec2 pos, const T& value, const Vec3& color = DEFAULT_COLOR, float height = 0.1f) -> void;

	// For enums could automatically get min max using COUNT enum value and get size from sizeof.
	auto debugImage(const ImageRgba* img) -> void;
	auto debugU8Array2d(u8* data, Vec2T<i64> size, u8 min, u8 max, bool posXGoingRight, bool posYGoingUp) -> void;
	auto debugF32Array2d(float* data, Vec2T<i64> size, float min, float max, bool posXGoingRight, bool posYGoingUp) -> void;
	// TODO: Send message (json object pointer maybe) assosiated with a point. On click open an imgui window. This could be a more general version of the physics engine inspector idea.

	struct Line {
		Vec2 start;
		Vec2 end;
		Vec3 color;
	};

	struct Circle {
		Vec2 pos;
		float radius;
		Vec3 color;
	};

	// Points always remain the same size, circles don't.
	struct Point {
		Vec2 pos;
		float radius;
		Vec3 color;
	};

	struct OrientedCircle {
		Circle circle;
		float orientation;
	};

	struct Parabola {
		float a;
		Vec2 pos;
		Vec3 color;
	};

	struct Triangle {
		::Triangle triangle;
		Vec3 color;
	};

	struct Text {
		Vec2 pos;
		const char* text;
		Vec3 color;
		float height;
	};

	static SimplePolygonTriangulator triangulate;
	extern std::vector<Line> lines;
	extern std::vector<Circle> circles;
	extern std::vector<Point> points;
	extern std::vector<OrientedCircle> circleColliders;
	extern std::vector<Circle> hollowCircles;
	extern std::vector<Parabola> parabolas;
	extern std::vector<Triangle> triangles;
	extern std::vector<Text> text;

	template<typename T>
	auto drawText(Vec2 pos, const T& value, const Vec3& color, float height) -> void {
		std::stringstream s;
		s << value;
		drawStr(pos, s.str().data(), color, height);
	}
}