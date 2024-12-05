#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

//typedef float float32_t;
//typedef double float64_t;


#define force_inline __forceinline

force_inline int clampi(int val, int min, int max) {
	const int tmp = val < min ? min : val;
	return tmp > max ? max : tmp;
}
force_inline float clampf(float val, float min, float max) {
	const float tmp = val < min ? min : val;
	return tmp > max ? max : tmp;
}

#define sign(x) (((x) > 0) - ((x) < 0))
force_inline int signi(int x) {
	return (x > 0) - (x < 0);
}
force_inline int signf(float x) {
	return (x > 0) - (x < 0);
}

//Straight from assert.h
#ifdef NDEBUG
#define assert_text(expression, details) ((void)0)
#else
#define assert_text(expression, details) (void)(																\
            (!!(expression)) ||																					\
            (_wassert(_CRT_WIDE(details ## ": " ## #expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0)	\
        )
#endif

