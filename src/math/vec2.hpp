#pragma once

#include <utils/asserts.hpp>
#include <utils/int.hpp>

#include <algorithm>
#include <cmath>

template<typename T>
struct Vec2T {
	constexpr Vec2T();
	constexpr explicit Vec2T(T all);
	constexpr Vec2T(T x, T y);
	static auto oriented(T angle) -> Vec2T;

	auto lengthSq() const -> T;
	auto length() const -> T;
	auto normalized() const -> Vec2T;
	auto rotBy90deg() const -> const Vec2T;
	auto applied(T (*function)(T)) const -> Vec2T;
	auto min(const Vec2T& other) const -> Vec2T;
	auto max(const Vec2T& other) const -> Vec2T;

	auto operator+(const Vec2T& v) const -> Vec2T;
	auto operator+=(const Vec2T& v) -> Vec2T&;
	auto operator-(const Vec2T& v) const -> Vec2T;
	auto operator-=(const Vec2T& v) -> Vec2T&;
	auto operator*(T s) const -> Vec2T;
	auto operator*=(T s) -> Vec2T&;
	auto operator*(const Vec2T& v) const -> Vec2T;
	auto operator/(T s) const -> Vec2T;
	auto operator/=(T s) -> Vec2T;
	auto operator/(const Vec2T& v) const -> Vec2T;
	auto operator/=(const Vec2T& v) -> Vec2T&;
	auto operator-() const -> Vec2T;

	auto operator[](isize index) -> T&;
	auto operator[](isize index) const -> const T&;

	// Not using <=> because it doesn't make sense for the other operators.
	auto operator==(const Vec2T& other) const -> bool;
	auto operator!=(const Vec2T& other) const -> bool;

	auto data() -> T*;

	T x, y;
};

using Vec2 = Vec2T<float>;

template<typename T>
auto operator*(T s, const Vec2T<T>& v) -> Vec2T<T> {
	return v * s;
}

template<typename T>
auto operator/(T s, const Vec2T<T>& v) -> Vec2T<T> {
	return Vec2{ s / v.x, s / v.y };
}

template<typename T>
auto dot(const Vec2T<T>& a, const Vec2T<T> b) -> T {
	return a.x * b.x + a.y * b.y;
}

template<typename T>
auto det(const Vec2T<T>& a, const Vec2T<T> b) -> T {
	return a.x * b.y - b.x * a.y;
}

template<typename T>
auto distance(const Vec2T<T>& a, const Vec2T<T> b) -> T {
	return (a - b).length();
}

// I don't think there are any good ways to leaves values unitialized explicitly so I just have to use the default constrcutor. Leaving the values unitialized is useful when creating an array. This could also be solved by providing some intialization function or in some cases maybe using a statically sized vector.
template<typename T>
constexpr Vec2T<T>::Vec2T()
	: x(T(123123123))
	, y(T(123123123)) {}

template<typename T>
constexpr Vec2T<T>::Vec2T(T all)
	: x(all)
	, y(all) {}

template<typename T>
constexpr Vec2T<T>::Vec2T(T x, T y)
	: x(x)
	, y(y) {}

template<typename T>
auto Vec2T<T>::oriented(T angle) -> Vec2T {
	return Vec2{ cos(angle), sin(angle) };
}

template<typename T>
auto Vec2T<T>::lengthSq() const -> T {
	return x * x + y * y;
}

template<typename T>
auto Vec2T<T>::length() const -> T {
	return sqrtf(lengthSq());
}

template<typename T>
auto Vec2T<T>::normalized() const -> Vec2T {
	const auto l = length();
	if (l == 0) {
		return *this;
	}
	return *this / l;
}

template<typename T>
auto Vec2T<T>::rotBy90deg() const -> const Vec2T {
	return Vec2{ y, -x };
}

template<typename T>
auto Vec2T<T>::applied(T (*function)(T)) const -> Vec2T {
	return Vec2{ function(x), function(y) };
}

template<typename T>
auto Vec2T<T>::min(const Vec2T& other) const -> Vec2T {
	return Vec2T{ std::min(x, other.x), std::min(y, other.y) };
}

template<typename T>
auto Vec2T<T>::max(const Vec2T& other) const -> Vec2T {
	return Vec2T{ std::max(x, other.x), std::max(y, other.y) };
}

template<typename T>
auto Vec2T<T>::operator+(const Vec2T& v) const -> Vec2T {
	return Vec2T{ x + v.x, y + v.y };
}

template<typename T>
auto Vec2T<T>::operator+=(const Vec2T& v) -> Vec2T& {
	*this = *this + v;
	return *this;
}

template<typename T>
auto Vec2T<T>::operator-(const Vec2T& v) const -> Vec2T {
	return Vec2T{ x - v.x, y - v.y };
}

template<typename T>
auto Vec2T<T>::operator-=(const Vec2T& v) -> Vec2T& {
	*this = *this - v;
	return *this;
}

template<typename T>
auto Vec2T<T>::operator*(T s) const -> Vec2T {
	return Vec2T{ x * s, y * s };
}

template<typename T>
auto Vec2T<T>::operator*=(T s) -> Vec2T& {
	*this = *this * s;
	return *this;
}

template<typename T>
auto Vec2T<T>::operator*(const Vec2T& v) const -> Vec2T {
	return Vec2T{ x * v.x, y * v.y };
}

template<typename T>
auto Vec2T<T>::operator/(T s) const -> Vec2T
{
	return Vec2T{ x / s, y / s };
}

template<typename T>
auto Vec2T<T>::operator/=(T s) -> Vec2T {
	*this = *this / s;
	return *this;
}

template<typename T>
auto Vec2T<T>::operator/(const Vec2T& v) const -> Vec2T {
	return Vec2T{ x / v.x, y / v.y };
}

template<typename T>
auto Vec2T<T>::operator/=(const Vec2T& v) -> Vec2T& {
	*this = *this / v;
	return *this;
}

template<typename T>
auto Vec2T<T>::operator-() const -> Vec2T {
	return Vec2T{ -x, -y };
}

template<typename T>
auto Vec2T<T>::operator[](isize index) -> T& {
	return const_cast<T&>((const_cast<const Vec2&>(*this))[index]);
}

template<typename T>
auto Vec2T<T>::operator[](isize index) const -> const T& {
	switch (index) {
	case 0: return x;
	case 1: return y;
	default:
		ASSERT_NOT_REACHED();
		return *reinterpret_cast<const T*>(nullptr);
	}
}

template<typename T>
auto Vec2T<T>::operator==(const Vec2T& other) const -> bool {
	return (x == other.x) && (y == other.y);
}

template<typename T>
auto Vec2T<T>::operator!=(const Vec2T& other) const -> bool {
	return !(*this == other);
}

template<typename T>
auto Vec2T<T>::data() -> T* {
	return &x;
}
