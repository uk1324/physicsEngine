#pragma once

#include <utils/asserts.hpp>
#include <utils/int.hpp>

template<typename T>
struct Vec2T {
	constexpr Vec2T();
	constexpr explicit Vec2T(T all);
	constexpr Vec2T(T x, T y);

	auto operator+(const Vec2T& v) const -> Vec2T;
	auto operator+=(const Vec2T& v) -> Vec2T&;
	auto operator/(T s) const -> Vec2T;
	auto operator/(const Vec2T& v) const -> Vec2T;
	auto operator/=(const Vec2T& v) -> Vec2T&;

	auto operator[](isize index) -> T&;
	auto operator[](isize index) const -> const T&;

	// Not using <=> because it doesn't make sense for the other operators.
	auto operator==(const Vec2T& other) const -> bool;
	auto operator!=(const Vec2T& other) const -> bool;

	T x, y;
};

using Vec2 = Vec2T<float>;

// I don't think there are any good ways to leaves values unitialized explicitly so I just have to use the default constrcutor.
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
auto Vec2T<T>::operator+(const Vec2T& v) const -> Vec2T
{
	return Vec2{ x + v.x, y + v.y };
}

template<typename T>
auto Vec2T<T>::operator+=(const Vec2T& v) -> Vec2T&
{
	*this = *this + v;
	return *this;
}

template<typename T>
auto Vec2T<T>::operator/(T s) const -> Vec2T
{
	return Vec2{ x / s, y / s };
}

template<typename T>
auto Vec2T<T>::operator/(const Vec2T& v) const -> Vec2T {
	return Vec2{ x / v.x, y / v.y };
}

template<typename T>
auto Vec2T<T>::operator/=(const Vec2T& v) -> Vec2T& {
	*this = *this / v;
	return *this;
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
		return T(123123123);
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
