#include "bitset.h"

#include <stdlib.h>
#include <string.h>


void bitset_clear(bitset_t* bitset, size_t size) {
	memset(bitset, 0, size * sizeof(bitset_t));
}
bool bitset_get(bitset_t* bitset,size_t index) {
	return (bitset[index / BITSET_WORD_SIZE] >> (index % BITSET_WORD_SIZE)) & 1;
}
void bitset_set(bitset_t* bitset, size_t index, bool bit) {
	bitset[index / BITSET_WORD_SIZE] = (bitset[index / BITSET_WORD_SIZE] & ~((size_t)1 << (index % BITSET_WORD_SIZE))) | ((size_t)bit << (index % BITSET_WORD_SIZE));
}
void bitset_set_weak(bitset_t* bitset, size_t index, bool bit) {
	bitset[index / BITSET_WORD_SIZE] |= (size_t)bit << (index % BITSET_WORD_SIZE);
}

typedef enum {
	_BITSET_DYN_CAPACITY_AT = 0,
	_BITSET_DYN_HEADER_SIZE_T_COUNT
} bitset_dyn_header_e;
#define _BITSET_DYN_DEFAULT_CAPACITY 8
#define _BITSET_DYN_GET_START(bitset) ((bitset) - _BITSET_DYN_HEADER_SIZE_T_COUNT)

bitset_t* bitset_dyn_new_size(size_t size) {
	size = BITSET_SIZE_ARRAY(size);
	if(size < 0) {
		size = 0;
	}
	bitset_t* tmp = malloc(_BITSET_DYN_HEADER_SIZE_T_COUNT * sizeof(bitset_t) + size * sizeof(bitset_t));
	tmp[_BITSET_DYN_CAPACITY_AT] = size;
	tmp = tmp + _BITSET_DYN_HEADER_SIZE_T_COUNT;
	memset(tmp, 0, size * sizeof(bitset_t));
	return tmp;
}
bitset_t* bitset_dyn_new(void) {
	return bitset_dyn_new_size(_BITSET_DYN_DEFAULT_CAPACITY * BITSET_WORD_SIZE);
}
void bitset_dyn_free(bitset_t* bitset) {
	free(_BITSET_DYN_GET_START(bitset));
}
size_t bitset_dyn_get_capacity(bitset_t* bitset) {
	return _BITSET_DYN_GET_START(bitset)[_BITSET_DYN_CAPACITY_AT];
}

bitset_t* bitset_dyn_expand_to(bitset_t* bitset, size_t to) {
	size_t capacity = bitset_dyn_get_capacity(bitset);
	size_t initial_capacity = capacity;
	while(to >= capacity) {
		capacity *= 2;
	}
	bitset = realloc(_BITSET_DYN_GET_START(bitset), _BITSET_DYN_HEADER_SIZE_T_COUNT * sizeof(bitset_t) + capacity * sizeof(bitset_t));
	bitset[_BITSET_DYN_CAPACITY_AT] = capacity;
	bitset = bitset + 1;
	memset(bitset + initial_capacity, 0, (capacity - initial_capacity) * sizeof(bitset_t));
	return bitset;
}

bitset_t* _bitset_dyn_set(bitset_t* bitset, size_t index, bool bit) {
	bitset_dyn_expand_to(bitset, index / BITSET_WORD_SIZE);
	bitset_set(bitset, index, bit);
	return bitset;
}
bitset_t* _bitset_dyn_set_weak(bitset_t* bitset, size_t index, bool bit) {
	bitset_dyn_expand_to(bitset, index / BITSET_WORD_SIZE);
	bitset_set_weak(bitset, index, bit);
	return bitset;
}
bool bitset_dyn_get(bitset_t* bitset, size_t index) {
	return bitset_get(bitset, index);
}
void bitset_dyn_clear(bitset_t* bitset) {
	bitset_clear(bitset, bitset_dyn_get_capacity(bitset));
}
