#pragma once

template<typename T>
constexpr T PI = static_cast<T>(3.14159265359);
template<typename T>
constexpr T TAU = PI<T> * static_cast<T>(2.0);

static constexpr float SIGNS[]{ -1.0f, 1.0f };

template<typename T, typename U>
auto lerp(T a, T b, U t) -> T {
	// This is better than a + (b - a) * t because it isn't as affected by rounding errors.
	return a * (1 - t) + b * t;
}