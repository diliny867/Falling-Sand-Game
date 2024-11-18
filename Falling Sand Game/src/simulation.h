#pragma once

#include <assert.h>

#include "common.h"
#include "materials_common.h"
#include "../include/bitset.h"
#include "../resources/ALL_MATERIALS.h"
#include "../resources/shaders/glsl_common.h"
#include "../include/arena.h"
#include "../include/rand_xorshift.h"

// dont change this one (or then edit DATA_MAP_GET_I)
#define GRID_DATA_MAP_CELL_RES 4
#define GRID_CELLS_WIDTH 250
#define GRID_CELLS_HEIGHT 250
#define GRID_CELLS_SIZE (GRID_CELLS_WIDTH * GRID_CELLS_HEIGHT)

#define GRID_WIDTH (GRID_CELLS_WIDTH * GRID_DATA_MAP_CELL_RES)
#define GRID_HEIGHT (GRID_CELLS_HEIGHT * GRID_DATA_MAP_CELL_RES)
#define GRID_SIZE (GRID_WIDTH * GRID_HEIGHT)


#define SPEED_MAX UINT8_MAX
#define DEATH_CHANCE_MAX UINT8_MAX
#define DENSITY_MAX UINT8_MAX

#define ARRAY2D_GET_I(x, y, width) ((y) * (width) + (x))
#define GRID_GET_I(x, y) ARRAY2D_GET_I((x), (y), GRID_WIDTH)
#define INTERACTION_GET_I(mat1, mat2) ARRAY2D_GET_I((mat1), (mat2), MATERIALS_COUNT)
#define DATA_MAP_GET_I(x, y) ARRAY2D_GET_I((x) >> 2, (y) >> 2, GRID_CELLS_WIDTH)
//#define GRID_INDEX_GET_Y(index) (index) / GRID_WIDTH
//#define GRID_INDEX_GET_X(index) (index) % GRID_WIDTH
#define GRID_INDEX_GET_XY(index, x, y) (y) = (index) / GRID_WIDTH; (x) = (index) - (y) * GRID_WIDTH

// Outer bounds do/dont stop particles, TODO: implement 
#define BOUNDS_PASS_THROUGH


typedef struct {
	int life;
	float x, y;
	float vx, vy;
    float t;
    uint32_t flags;
} pdata_t;

typedef enum {
	COLL_CANT_MOVE = 0,
	COLL_SWAP,
	COLL_REPLACE,
} collision_res_e_t;

typedef struct {
	collision_res_e_t collision;
	float bounce;
} interaction_t;

typedef struct {
	float x, y;
} xy_t;

typedef struct simulation_t {
	material_type_e_t grid[GRID_SIZE];
	pdata_t particles[GRID_SIZE];
	int index_shuffle[GRID_SIZE];
	material_t materials[MATERIALS_COUNT];
	interaction_t interactions[MATERIALS_COUNT * MATERIALS_COUNT];
	bitset_t updated_cells_bitset[BITSET_SIZE_ARRAY(GRID_SIZE)];
	arena_t* arena;
	xorshift32_state rand_state;
	//float gravity_map[GRID_CELLS_SIZE];
	float gravity;
	float air_drag;
	float max_v;
	float pressure_map[GRID_CELLS_SIZE];
	xy_t velocity_map[GRID_CELLS_SIZE];
} simulation_t;


simulation_t* simulation_new(arena_t* a);

void simulation_init(simulation_t* sim);

void simulation_tick(simulation_t* sim);

void simulation_place(simulation_t* sim, material_type_e_t material, int x, int y, int size, int scatter, bool round);

void simulation_free(simulation_t* sim);