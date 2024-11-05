#pragma once

#include <stdint.h>


typedef struct {
	uint32_t a;
} xorshift32_state;

inline uint32_t xorshift32(xorshift32_state* state){
	uint32_t x = state->a;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return state->a = x;
}


typedef struct {
	uint64_t a;
} xorshift64_state;

inline uint64_t xorshift64(xorshift64_state* state){
	uint64_t x = state->a;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	return state->a = x;
}


typedef struct {
	uint32_t x[4];
} xorshift32_128_state;

inline uint32_t xorshift32_128(xorshift32_128_state* state){
	uint32_t t  = state->x[3];

	uint32_t s  = state->x[0];
	state->x[3] = state->x[2];
	state->x[2] = state->x[1];
	state->x[1] = s;

	t ^= t << 11;
	t ^= t >> 8;
	return state->x[0] = t ^ s ^ (s >> 19);
}