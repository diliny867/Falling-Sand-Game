#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common.h"
#include "bitset.h"
#include "include/arena.h"
#include "include/rand_xorshift.h"

#define GRID_WIDTH 1024
#define GRID_HEIGHT 1024
#define GRID_SIZE (GRID_WIDTH * GRID_HEIGHT)

typedef enum {
	AIR = 0,
	CONCRETE,
	WATER,
	SAND,
	ROCK,
	WOOD,
	FIRE,
	MATERIALS_COUNT
} material_type_e_t;

typedef enum {
	DOWN		= 0x1,
	UP			= 0x2,
	SIDES		= 0x4
} flag_direction_e_t;

typedef float float32_t;
typedef double float64_t;

typedef struct {
	uint8_t r, g, b, a;
} rgba_t;

typedef struct {
	rgba_t color;
	uint8_t speed;
	uint8_t direction;
	uint8_t death_chance; // survivability chance: 0 to 255 (255 is 100%)
	uint8_t density;
	bool solid;
	bool flamable;
	bool flaming;
} material_t;

#define SPEED_MAX UINT8_MAX
#define DEATH_CHANCE_MAX UINT8_MAX
#define DENSITY_MAX UINT8_MAX


typedef struct {
	material_type_e_t grid[GRID_SIZE];
	int index_shuffle[GRID_SIZE];
	material_t materials[MATERIALS_COUNT];
	bitset_t updated_cells_bitset[BITSET_SIZE_ARRAY(GRID_SIZE)];
	arena_t* arena;
	xorshift32_state rand_state;
} fsgame_t; // falling_sand_game_t


fsgame_t* game_new(arena_t* a);

void game_init(fsgame_t* game);

void game_tick(fsgame_t* game);

void game_place(fsgame_t* game, material_type_e_t material, int x, int y, int size, int scatter, bool round);