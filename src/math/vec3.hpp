#pragma once

template<typename T>
struct Vec3T {
	constexpr Vec3T(const T& x, const T& y, const T& z);

	T x, y, z;
};

using Vec3 = Vec3T<float>;

template<typename T>
constexpr Vec3T<T>::Vec3T(const T& x, const T& y, const T& z) 
	: x(x) 
	, y(y)
	, z(z) {}