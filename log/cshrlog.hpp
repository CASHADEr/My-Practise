#pragma once
#ifdef __cshr_debug__
#include <stdio.h>
#define cshrlog(...) printf(__VA_ARGS__)
#else
#define cshrlog(...)
#endif