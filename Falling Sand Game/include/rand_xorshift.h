#pragma once

#include "xorshift.h"

inline uint32_t xorshift32_n(xorshift32_state* state, uint32_t n) {
	return xorshift32(state) % n;
}
inline uint32_t xorshift32_range(xorshift32_state* state, uint32_t from, uint32_t to) {
	return from + xorshift32(state) % (to - from);
}

inline uint64_t xorshift64_n(xorshift64_state* state, uint64_t n){
	return xorshift64(state) % n;
}
inline uint64_t xorshift64_range(xorshift64_state* state, uint64_t from, uint64_t to){
	return from + xorshift64(state) % (to - from);
}

inline uint32_t xorshift32_128_n(xorshift32_128_state* state, uint32_t n){
	return xorshift32_128(state) % n;
}
inline uint32_t xorshift32_128_range(xorshift32_128_state* state, uint32_t from, uint32_t to){
	return from + xorshift32_128(state) % (to - from);
}