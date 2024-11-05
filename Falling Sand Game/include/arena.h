#pragma once

#include <stdlib.h>
#include <stdint.h>


typedef struct arena_chunk_t {
    struct arena_chunk_t* next;
    size_t size;
    size_t cap;
    uint8_t data[];
} arena_chunk_t;

typedef struct {
    arena_chunk_t* start;
    arena_chunk_t* end;
} arena_t;


arena_t* arena_new();
void arena_free(arena_t* a);

void* arena_alloc(arena_t* a, size_t size);
void* arena_realloc(arena_t* a, void* ptr, size_t old_size, size_t new_size);