#pragma once

#include <math/mat3x2.hpp>

struct float3x2 {
	float3x2(const Mat3x2& mat);

private:
	alignas(16) float x[3];
	alignas(16) float y[3];
};