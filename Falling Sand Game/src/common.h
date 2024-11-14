#pragma once

#include <stdint.h>

typedef float float32_t;
typedef double float64_t;


#define force_inline __forceinline inline

force_inline int clampi(int val, int min, int max) {
	if(val < min) {
		return min;
	}
	if(val > max) {
		return max;
	}
	return val;
}

#define sign(x) ((x > 0) - (x < 0))
