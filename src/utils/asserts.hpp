#pragma once

#ifdef _DEBUG
#define ASSERT(expr) do { if (!(expr)) { __debugbreak(); } } while (false)
#else
#define ASSERT(expr)
#endif 

#define ASSERT_NOT_REACHED() ASSERT(false)