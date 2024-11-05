#pragma once

#include <stdbool.h>
#include <stdint.h>

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
	LEFT		= 0x4,
	RIGHT		= 0x8,
} flag_direction_e_t;

typedef float float32_t;
typedef double float64_t;

typedef struct {
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
	material_t materials[MATERIALS_COUNT];
	arena_t* arena;
	xorshift32_state rand_state;
} fsgame_t; // falling_sand_game_t


fsgame_t* new_game(arena_t* a);

void init_game(fsgame_t* game);

void tick(fsgame_t* game);