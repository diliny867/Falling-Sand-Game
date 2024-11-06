#include "bitset.h"

#include <stdlib.h>
#include <string.h>


#define _BITSET_CAPACITY_AT 0
#define _BITSET_HEADER_SIZE_T_COUNT 1

#define _BITSET_DEFAULT_CAPACITY 8

#define _BITSET_WORD_SIZE (sizeof(size_t) * 8)

#define _BITSET_GET_START(bitset) ((bitset) - _BITSET_HEADER_SIZE_T_COUNT)


size_t* bitset_new_size(size_t size) {
	size /= _BITSET_WORD_SIZE;
	if(size < 0) {
		size = 0;
	}
	size_t* tmp = malloc(_BITSET_HEADER_SIZE_T_COUNT * sizeof(size_t) + size * sizeof(size_t));
	tmp[_BITSET_CAPACITY_AT] = size;
	tmp = tmp + _BITSET_HEADER_SIZE_T_COUNT;
	memset(tmp, 0, size * sizeof(size_t));
	return tmp;
}
size_t* bitset_new(void) {
	return bitset_new_size(_BITSET_DEFAULT_CAPACITY * _BITSET_WORD_SIZE);
}
void bitset_free(size_t* bitset) {
	free(_BITSET_GET_START(bitset));
}

static size_t* _bitset_new_double_cap(size_t* bitset) {
	size_t capacity = _BITSET_GET_START(bitset)[_BITSET_CAPACITY_AT];
	bitset = realloc(_BITSET_GET_START(bitset), _BITSET_HEADER_SIZE_T_COUNT * sizeof(size_t) + capacity * 2 * sizeof(size_t));
	bitset[_BITSET_CAPACITY_AT] = capacity * 2;
	bitset = bitset + 1;
	memset(bitset + capacity, 0, capacity * sizeof(size_t));
	return bitset;
}

size_t* _bitset_set(size_t* bitset, size_t index, bool bit) {
	size_t uint_index = index / _BITSET_WORD_SIZE;
	size_t capacity;
	while(uint_index >= (capacity = _BITSET_GET_START(bitset)[_BITSET_CAPACITY_AT])) {
		bitset = realloc(_BITSET_GET_START(bitset), _BITSET_HEADER_SIZE_T_COUNT * sizeof(size_t) + capacity * 2 * sizeof(size_t));
		bitset[_BITSET_CAPACITY_AT] = capacity * 2;
		bitset = bitset + 1;
		memset(bitset + capacity, 0, capacity * sizeof(size_t));
	}
	bitset[index / _BITSET_WORD_SIZE] &= ~(!bit << (index % _BITSET_WORD_SIZE));
	return bitset;
}
size_t* _bitset_set_weak(size_t* bitset, size_t index, bool bit) {
	size_t uint_index = index / _BITSET_WORD_SIZE;
	size_t capacity;
	while(uint_index >= (capacity = _BITSET_GET_START(bitset)[_BITSET_CAPACITY_AT])) {
		bitset = realloc(_BITSET_GET_START(bitset), _BITSET_HEADER_SIZE_T_COUNT * sizeof(size_t) + capacity * 2 * sizeof(size_t));
		bitset[_BITSET_CAPACITY_AT] = capacity * 2;
		bitset = bitset + 1;
		memset(bitset + capacity, 0, capacity * sizeof(size_t));
	}
	bitset[index / _BITSET_WORD_SIZE] |= bit << (index % _BITSET_WORD_SIZE);
	return bitset;
}
bool bitset_get(size_t* bitset, size_t index) {
	return (bitset[index / _BITSET_WORD_SIZE] >> (index % _BITSET_WORD_SIZE)) & 1;
}
