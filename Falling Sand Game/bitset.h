#pragma once

#include <stdbool.h>
#include <stdint.h>

size_t* bitset_new(void);
size_t* bitset_new_size(size_t size);
void bitset_free(size_t* bitset);

size_t* _bitset_set(size_t* bitset, size_t index, bool bit);
size_t* _bitset_set_weak(size_t* bitset, size_t index, bool bit);
bool bitset_get(size_t* bitset, size_t index);

#define bitset_set(bitset, index, bit) \
	do { \
		(bitset) = _bitset_set((bitset), (index), (bit)); \
	} while(0)
#define bitset_set_weak(bitset, index, bit) \
	do { \
		(bitset) = _bitset_set_weak((bitset), (index), (bit)); \
	} while(0)
