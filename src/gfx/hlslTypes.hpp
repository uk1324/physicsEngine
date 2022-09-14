#pragma once

#include <math/mat3x2.hpp>

#pragma warning(push)
// MSVC doesn't like using allignment specifiers and issues a warning.
#pragma warning(disable : 4324)

// Structures don't need to be aligned on 16 byte boundries when used inside a constant buffer only their members need to be aligned relative to eachother. Using alignas is just simpler than inserting padding bytes manually which would probably still require using alignas for it to be consistent across compilers and architectures.

struct float3x2 {
	float3x2(const Mat3x2& mat);

private:
	alignas(16) float x[3];
	alignas(16) float y[3];
};

struct float3 {
	float3(const Vec3& vec);

private:
	alignas(16) float v[3];
};

#pragma warning(pop)