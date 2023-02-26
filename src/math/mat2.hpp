#pragma once

#include <math/vec2.hpp>
#include <utils/int.hpp>

// Row major
template<typename T>
struct Mat2T {
	Mat2T() = default;
	constexpr Mat2T(const Vec2T<T>& x, const Vec2T<T>& y);

	static auto rotate(T angle) -> Mat2T;

	auto transposed() const -> Mat2T;
	auto orthonormalInv() const -> Mat2T;
	auto inversed() const -> Mat2T;

	auto det() const -> float;
	
	auto x() const -> Vec2T<T>;
	auto y() const -> Vec2T<T>;

	auto operator*(const Mat2T& other) const -> Mat2T;
	auto operator[](isize i);
	auto operator[](isize i) const;

	static constexpr auto COLS = 2;
	static constexpr int ROWS = 2;
	float m[ROWS][COLS];
	static const Mat2T identity;
};

using Mat2 = Mat2T<float>;

template<typename T>
auto operator*(const Vec2T<T>& v, const Mat2T<T>& m) -> Vec2T<T>;
template<typename T>
auto operator*=(Vec2T<T>& v, const Mat2T<T>& m) -> Vec2T<T>&;

template<typename T>
constexpr Mat2T<T>::Mat2T(const Vec2T<T>& x, const Vec2T<T>& y)
	: m{ { x.x, x.y }, { y.x, y.y } } {
}

template<typename T>
auto Mat2T<T>::rotate(T angle) -> Mat2T {
	const auto s = sin(angle), c = cos(angle);
	return Mat2T{ Vec2{ c, s }, Vec2{ -s, c } };
}

template<typename T>
auto Mat2T<T>::transposed() const -> Mat2T {
	const auto& n = *this;
	return Mat2T{
		Vec2T{ n[0][0], n[1][0] },
		Vec2T{ n[0][1], n[1][1] }
	};
}

template<typename T>
auto Mat2T<T>::orthonormalInv() const -> Mat2T {
	return transposed();
}

template<typename T>
auto Mat2T<T>::inversed() const -> Mat2T {
	const auto det = m[0][0] * m[1][1] - m[0][1] * m[1][0];
	if (det == 0.0f) {
		ASSERT_NOT_REACHED();
	}

	return Mat2T{
		Vec2T{ m[1][1] / det, -m[1][0] / det },
		Vec2T{ -m[0][1] / det, m[0][0] / det },
	};
}

template<typename T>
auto Mat2T<T>::det() const -> float {
	return m[0][0] * m[1][1] - m[1][0] * m[0][1];
}

template<typename T>
auto Mat2T<T>::x() const -> Vec2T<T> {
	return Vec2T{ m[0][0], m[0][1] };
}

template<typename T>
auto Mat2T<T>::y() const -> Vec2T<T> {
	return Vec2T{ m[1][0], m[1][1] };
}

template<typename T>
auto Mat2T<T>::operator*(const Mat2T& other) const -> Mat2T {
	return Mat2{
		x() * other,
		y() * other,
	};
}

template<typename T>
auto Mat2T<T>::operator[](isize i) {
	return m[i];
}

template<typename T>
auto Mat2T<T>::operator[](isize i) const {
	return m[i];
}

template<typename T>
auto operator*(const Vec2T<T>& v, const Mat2T<T>& m) -> Vec2T<T> {
	return Vec2{
		v.x * m[0][0] + v.y * m[1][0],
		v.x * m[0][1] + v.y * m[1][1]
	};
}

template<typename T>
auto operator*=(Vec2T<T>& v, const Mat2T<T>& m) -> Vec2T<T>& {
	v = v * m;
	return v;
}

template<typename T>
const Mat2T<T> Mat2T<T>::identity = Mat2T<T>{ Vec2T<T>{ 1, 0 }, Vec2T<T>{ 0, 1 } };
