#pragma once

#include <assert.h>


// TODO: Rewrite to allow enabling asserts in release mode.
#define ASSERT(expr) assert(expr)
#define ASSERT_NOT_REACHED(expr) assert(false)