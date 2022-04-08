#pragma once
#ifdef __cshr_debug__
#include <stdio.h>
#include <assert.h>
#define cshrlog(...) printf(__VA_ARGS__)
#define cshrassert(x) assert(x)
#else
#define cshrlog(...)
#define cshrassert(x)
#endif