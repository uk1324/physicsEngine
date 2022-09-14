#include <gfx/hlslTypes.hpp>

float3x2::float3x2(const Mat3x2& mat) 
	: x{ mat[0][0], mat[1][0], mat[2][0] }
	, y{ mat[0][1], mat[1][1], mat[2][1] } {
}

float3::float3(const Vec3& vec) 
	: v{ vec.x, vec.y, vec.z } {
}