#pragma once

#include <math/vec2.hpp>
#include <utils/int.hpp>

template<typename T>
struct Mat2T {
	Mat2T() = default;
	constexpr Mat2T(const Vec2T<T>& x, const Vec2T<T>& y);

	auto det() const -> float;

	auto operator[](isize i);
	auto operator[](isize i) const;

	static constexpr auto COLS = 2;
	static constexpr int ROWS = 2;
	float m[ROWS][COLS];
};

using Mat2 = Mat2T<float>;

template<typename T>
constexpr Mat2T<T>::Mat2T(const Vec2T<T>& x, const Vec2T<T>& y)
	: m{ { x.x, x.y }, { y.x, y.y } } {
}

template<typename T>
auto Mat2T<T>::det() const -> float {
	const auto& m = *this;
	return m[0][0] * m[1][1] - m[1][0] * m[0][1];
}

template<typename T>
auto Mat2T<T>::operator[](isize i) {
	return m[i];
}

template<typename T>
auto Mat2T<T>::operator[](isize i) const {
	return m[i];
}
