#pragma once

template<typename T>
constexpr T PI = static_cast<T>(3.14159265359);
template<typename T>
constexpr T TAU = PI<T> * static_cast<T>(2.0);
template<typename T>
constexpr T E = static_cast<T>(2.71828182846);

static constexpr float SIGNS[]{ -1.0f, 1.0f };

template<typename T, typename U>
auto lerp(T a, T b, U t) -> T {
	// This is better than a + (b - a) * t because it isn't as affected by rounding errors as much.
	return a * (1 - t) + b * t;
}

template<typename T>
auto sign(T x) -> T {
	return x < 0 ? static_cast<T>(-1) : static_cast<T>(1);
}

// To move 2 conntected lines with unconstrained roatation to a point you just need to construct a triangle with the side lengths(using law of sines or cosines). There are 2 ways to do this. Use the triangle sides sum law to check if this is possible or check if it lies in a hollowed out circle. 
// If there are 3 lines there are infinite solutions for example piston rod engines.