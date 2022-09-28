#pragma once

template<typename T>
constexpr T PI = T(3.14);
template<typename T>
constexpr T TAU = PI<T> * T(2);

static constexpr float SIGNS[]{ -1.0f, 1.0f };