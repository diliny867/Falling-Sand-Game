#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef size_t bitset_t;

#define BITSET_WORD_SIZE (sizeof(bitset_t) * 8)

#define BITSET_SIZE_ARRAY(size) (((size) - 1) / BITSET_WORD_SIZE + 1)

void bitset_clear(bitset_t* bitset, size_t size);
bool bitset_get(bitset_t* bitset, size_t index);
void bitset_set(bitset_t* bitset, size_t index, bool bit);
void bitset_set_weak(bitset_t* bitset, size_t index, bool bit);


bitset_t* bitset_dyn_new(void);
bitset_t* bitset_dyn_new_size(size_t size);
void bitset_dyn_free(bitset_t* bitset);
void bitset_dyn_clear(bitset_t* bitset);

size_t bitset_dyn_get_capacity(bitset_t* bitset);
bitset_t* bitset_dyn_expand_to(bitset_t* bitset, size_t to);

bitset_t* _bitset_dyn_set(bitset_t* bitset, size_t index, bool bit);
bitset_t* _bitset_dyn_set_weak(bitset_t* bitset, size_t index, bool bit);
bool bitset_dyn_get(bitset_t* bitset, size_t index);


#define bitset_dyn_set(bitset, index, bit) \
	do { \
		(bitset) = _bitset_dyn_set((bitset), (index), (bit)); \
	} while(0)
#define bitset_dyn_set_weak(bitset, index, bit) \
	do { \
		(bitset) = _bitset_dyn_set_weak((bitset), (index), (bit)); \
	} while(0)

