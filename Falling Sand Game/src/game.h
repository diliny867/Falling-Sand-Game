#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "common.h"
#include "../include/bitset.h"
#include "../resources/shaders/glsl_common.h"
#include "../include/arena.h"
#include "../include/rand_xorshift.h"
#include "raymath.h"

#define GRID_DATA_MAP_CELL_RES 4
#define GRID_CELLS_WIDTH 250
#define GRID_CELLS_HEIGHT 250
#define GRID_CELLS_SIZE (GRID_CELLS_WIDTH * GRID_CELLS_HEIGHT)

#define GRID_WIDTH (GRID_CELLS_WIDTH * GRID_DATA_MAP_CELL_RES)
#define GRID_HEIGHT (GRID_CELLS_HEIGHT * GRID_DATA_MAP_CELL_RES)
#define GRID_SIZE (GRID_WIDTH * GRID_HEIGHT)


#define ENUM_FOREACH_MATERIAL(OP) \
        OP(AIR)   \
        OP(CONCRETE)  \
        OP(WATER)   \
        OP(SAND)  \
        OP(ROCK)  \
        OP(WOOD)  \
        OP(FIRE)  \
        OP(MATERIALS_COUNT)

#define ENUM_GEN(val) val,
#define ENUM_GEN_STRING(val) #val,

typedef enum {
	ENUM_FOREACH_MATERIAL(ENUM_GEN)
} material_type_e_t;
static const char* MATERIAL_ENUM_STRING[] ={
	ENUM_FOREACH_MATERIAL(ENUM_GEN_STRING)
};

typedef enum {
	DOWN		= 0x1,
	UP			= 0x2,
	SIDES		= 0x4
} flag_direction_e_t;


typedef struct {
	uint8_t r, g, b, a;
} rgba_t;

typedef struct {
	rgba_t color;
	float density; // every particle has the same volume, so dont care about it
	float viscosity;
	uint8_t speed;
	uint8_t direction;
	uint8_t death_chance; // survivability chance: 0 to 255 (255 is 100%)
	bool solid;
	bool flamable;
	bool flaming;
} material_t;

#define SPEED_MAX UINT8_MAX
#define DEATH_CHANCE_MAX UINT8_MAX
#define DENSITY_MAX UINT8_MAX

typedef float temperature_t;
typedef enum {
	NONE = 0,
	BURNING = 1,
} material_flags_e_t;
_STATIC_ASSERT(NONE == MAT_FLAG_NONE);
_STATIC_ASSERT(BURNING == MAT_FLAG_BURNING);

typedef struct {
	material_type_e_t grid[GRID_SIZE];
	temperature_t temperatures[GRID_SIZE];
	Vector2 velocities[GRID_SIZE];
	Vector2 directions[GRID_SIZE];
	material_flags_e_t flags[GRID_SIZE];
	int index_shuffle[GRID_SIZE];
	material_t materials[MATERIALS_COUNT];
	bitset_t updated_cells_bitset[BITSET_SIZE_ARRAY(GRID_SIZE)];
	arena_t* arena;
	xorshift32_state rand_state;
	float gravity;
	float gravity_map[GRID_CELLS_SIZE];
	float pressure_map[GRID_CELLS_SIZE];
} fsgame_t; // falling_sand_game_t


fsgame_t* game_new(arena_t* a);

void game_init(fsgame_t* game);

void game_tick(fsgame_t* game);

void game_place(fsgame_t* game, material_type_e_t material, int x, int y, int size, int scatter, bool round);