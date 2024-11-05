#include "arena.h"

#include <string.h>

#define _ARENA_CHUNK_DEFAULT_CAPACITY (16384)


static arena_chunk_t* arena_chunk_new(size_t cap){
    arena_chunk_t* chunk;
    chunk = (arena_chunk_t*)malloc(sizeof(arena_chunk_t) + sizeof(uint8_t) * cap);
    chunk->cap = cap;
    chunk->size = 0;
    chunk->next = NULL;
    return chunk;
}
static void arena_chunk_free(arena_chunk_t* ch){
    free(ch);
}

arena_t* arena_new(){
    arena_t* a = (arena_t*)malloc(sizeof(arena_t));
    a->start = NULL;
    a->end = NULL;
    return a;
}
void arena_free(arena_t* a){
    arena_chunk_t* ch = a->start;
    arena_chunk_t* curr;
    while (ch != NULL){
        curr = ch;
        ch = ch->next;
        arena_chunk_free(curr);
    }
    free(a);
}

void* arena_alloc(arena_t* a, size_t size){
    size_t alloc_size = _ARENA_CHUNK_DEFAULT_CAPACITY;
    if(size > alloc_size){
        alloc_size = ((size - 1) / _ARENA_CHUNK_DEFAULT_CAPACITY + 1) * _ARENA_CHUNK_DEFAULT_CAPACITY;
    }

    arena_chunk_t* ch = a->start;

    if(ch == NULL){
        a->start = arena_chunk_new(alloc_size);
        ch = a->start;
        a->end = ch;
    }
    if(ch->size + size <= ch->cap){
        ch->size += size;
        return ch->data + ch->size - size;
    }

    while(ch->next != NULL && ch->next->size + size > ch->next->cap){
        ch = ch->next;
    }

    if(ch->next == NULL){
        ch->next = arena_chunk_new(alloc_size);
	    a->end = ch->next;
    }

    ch->next->size += size;
    return ch->next->data + ch->next->size - size;
}

void* arena_realloc(arena_t* a, void* ptr, size_t old_size, size_t new_size){
    if(new_size <= old_size){
        return ptr;
    }
    void* res = arena_alloc(a, new_size);
    memcpy(res, ptr, new_size);
    return res;
}