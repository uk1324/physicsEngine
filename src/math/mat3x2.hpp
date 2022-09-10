#pragma once

#include <math/vec2.hpp>
#include <math/vec3.hpp>
#include <utils/int.hpp>

template<typename T>
struct Mat3x2T {
	Mat3x2T() = default;
	constexpr Mat3x2T(const Vec2T<T>& x, const Vec2T<T>& y, const Vec2T<T>& w);

	auto operator*(const Mat3x2T& other) const -> Mat3x2T;
	auto operator[](isize i);
	auto operator[](isize i) const;

	static auto translate(const Vec2T<T> v) -> Mat3x2T;
	static auto scale(const Vec2T<T> v) -> Mat3x2T;
	static auto rotate(T angle) -> Mat3x2T;
	static const Mat3x2T identity;

	static constexpr auto COLS = 2;
	static constexpr int ROWS = 3;
	float m[ROWS][COLS];
};

using Mat3x2 = Mat3x2T<float>;

template<typename T>
auto operator*(const Vec3T<T>& v, const Mat3x2T<T>& m) -> Vec2T<T>;

template<typename T>
auto operator*(const Vec2T<T>& v, const Mat3x2T<T>& m) -> Vec2T<T>;

template<typename T>
constexpr Mat3x2T<T>::Mat3x2T(const Vec2T<T>& x, const Vec2T<T>& y, const Vec2T<T>& w)
	: m{ { x.x, x.y }, { y.x, y.y }, { w.x, w.y } } {
}

template<typename T>
auto Mat3x2T<T>::operator*(const Mat3x2T& other) const -> Mat3x2T {
	Mat3x2T<T> mat(
		Vec3{ m[0][0], m[0][1], 0 } * other,
		Vec3{ m[1][0], m[1][1], 0 } * other,
		Vec3{ m[2][0], m[2][1], 1 } * other
	);
	return mat;
}

template<typename T>
auto Mat3x2T<T>::operator[](isize i) {
	return m[i];
}

template<typename T>
auto Mat3x2T<T>::operator[](isize i) const {
	return m[i];
}

template<typename T>
auto Mat3x2T<T>::translate(const Vec2T<T> v) -> Mat3x2T {
	return Mat3x2T{ Vec2T<T>{ 1, 0 }, Vec2T<T>{ 0, 1 }, v };
}

template<typename T>
auto Mat3x2T<T>::scale(const Vec2T<T> v) -> Mat3x2T {
	return Mat3x2T{ Vec2T<T>{ v.x, 0 }, Vec2T<T>{ 0, v.y }, Vec2T<T>{ 0, 0 } };
}

template<typename T>
auto Mat3x2T<T>::rotate(T angle) -> Mat3x2T {
	return Mat3x2T{ 
		Vec2T<T>{ cosf(angle), sinf(angle) }, 
		Vec2T<T>{ -sinf(angle), cosf(angle) },
		Vec2T<T>{ 0, 0 } 
	};
}

template<typename T>
const Mat3x2T<T> Mat3x2T<T>::identity = Mat3x2T<T>(Vec2T<T>(1, 0), Vec2T<T>(0, 1), Vec2T<T>(0, 0));

template<typename T>
auto operator*(const Vec3T<T>& v, const Mat3x2T<T>& m) -> Vec2T<T> {
	return Vec2T<T>{ 
		v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0],
		v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1]
	};
}

template<typename T>
auto operator*(const Vec2T<T>& v, const Mat3x2T<T>& m) -> Vec2T<T> {
	return Vec3{ v.x, v.y, 1 } * m;
}