#include "game.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bitset.h"


fsgame_t* game_new(arena_t* a) {
	fsgame_t* game = arena_alloc(a, sizeof(fsgame_t));
	game->arena = a;
	return game;
}

#define SET_MATERIAL_DEFAULT(mat) \
	(mat).color = (rgba_t){0, 0, 0, 0};\
	(mat).speed = 0; \
	(mat).direction = 0; \
	(mat).death_chance = 0; \
	(mat).density = 0; \
	(mat).solid = false; \
	(mat).flamable = false; \
	(mat).flaming = false

#define SET_MATERIAL_DEFAULT_NONSOLID(mat) \
	SET_MATERIAL_DEFAULT(mat); \
	(mat).solid = false

#define SET_MATERIAL_DEFAULT_SOLID(mat) \
	SET_MATERIAL_DEFAULT(mat); \
	(mat).solid = true

static void init_materials(fsgame_t* game) {
	material_t* mat;

	mat = &game->materials[AIR];
	SET_MATERIAL_DEFAULT_NONSOLID(*mat);
	
	mat = &game->materials[CONCRETE];
	SET_MATERIAL_DEFAULT_SOLID(*mat);
	mat->color = (rgba_t){166, 166, 163, 255};
	mat->density = DENSITY_MAX;

	mat = &game->materials[WATER];
	SET_MATERIAL_DEFAULT_NONSOLID(*mat);
	mat->color = (rgba_t){98, 179, 222, 255};
	mat->direction |= DOWN;
	mat->speed = 1;
	mat->density = DENSITY_MAX * 0.3;
	
	mat = &game->materials[SAND];
	SET_MATERIAL_DEFAULT_SOLID(*mat);
	mat->color = (rgba_t){250, 242, 197, 255};
	mat->direction |= DOWN;
	mat->speed = 1;
	mat->density = DENSITY_MAX * 0.5;

	mat = &game->materials[ROCK];
	SET_MATERIAL_DEFAULT_SOLID(*mat);
	mat->color = (rgba_t){152, 113, 70, 255};
	mat->density = DENSITY_MAX * 0.65;

	mat = &game->materials[WOOD];
	SET_MATERIAL_DEFAULT_SOLID(*mat);
	mat->color = (rgba_t){187, 134, 71, 255};
	mat->density = DENSITY_MAX * 0.5;
	mat->flamable = true;

	mat = &game->materials[FIRE];
	SET_MATERIAL_DEFAULT_NONSOLID(*mat);
	mat->color = (rgba_t){255, 95, 8, 255};
	mat->direction |= UP;
	mat->speed = 1;
	mat->flaming = true;
}

#define SWAP(T, a, b) \
	do { \
		T tmp = (a); \
		(a) = (b); \
		(b) = tmp; \
	} while(0)

static void array_shuffle(int* arr, size_t size, xorshift32_state* rand_state) {
	if(size <= 1) {
		return;
	}
	for (size_t i = size - 1; i > 0; i--) {
		size_t j = xorshift32_n(rand_state, i + 1);
		SWAP(size_t, arr[i], arr[j]);
	}
}
void game_init(fsgame_t* game) {
	game->rand_state.a = time(NULL);

	memset(game->grid, 0, GRID_SIZE * sizeof(material_type_e_t));

	for(int i = 0; i < GRID_SIZE; i++) {
		game->index_shuffle[i] = i;
	}
	array_shuffle(game->index_shuffle, GRID_SIZE, &game->rand_state);

	//bitset_clear(game->updated_cells_bitset, GRID_SIZE);


	init_materials(game);
}

#define GRID_GET_I(x, y) ((y) * (GRID_WIDTH) + (x))

static inline bool outof_grid(int index, uint8_t speed, uint8_t direction) {
	return (direction & RIGHT && (index % GRID_WIDTH + speed) >= GRID_WIDTH) ||
		   (direction & LEFT  && (index % GRID_WIDTH - speed) < 0) ||
		   (direction & DOWN  && (index + speed * GRID_WIDTH) >= GRID_SIZE) ||
		   (direction & UP    && (index - speed * GRID_WIDTH) < 0);
}

static int clamp(int val, int min, int max) {
	if(val < min) {
		return min;
	}
	if(val > max) {
		return max;
	}
	return val;
}
static int clamp_grid_w(int val) {
	return clamp(val, 0, GRID_WIDTH - 1);
}
static int clamp_grid_h(int val) {
	return clamp(val, 0, GRID_HEIGHT - 1);
}

void game_place(fsgame_t* game, material_type_e_t material, int x, int y, int size, int scatter, bool round) {
#ifdef GRID_PLACE_PRE_CLAMP
	int half_size = size / 2;
	int x_to = clamp_grid_w(x + half_size);
	int y_to = clamp_grid_h(y + half_size);
	int dx, dy;
	for(int y0 = clamp_grid_h(y - half_size); y0 < y_to; y0++) {
		for(int x0 = clamp_grid_w(x - half_size); x0 < x_to; x0++) {
			if(round){
				dx = x0 - x;
				dy = y0 - y;
				if(sqrtf(dx * dx + dy * dy) > half_size) {
					continue;
				}
			}
			dx = xorshift32_n(&game->rand_state, scatter + 1);
			dy = xorshift32_n(&game->rand_state, scatter + 1);
			game->grid[GRID_GET_I(x0 + dx, y0 + dy)] = material;
		}
	}
#else
	const int half_size = size / 2;
	const int x_to = x + half_size;
	const int y_to = y + half_size;
	int dx, dy;
	for(int y0 = y - half_size; y0 < y_to; y0++) {
		for(int x0 = x - half_size; x0 < x_to; x0++) {
			if(round){
				dx = x0 - x;
				dy = y0 - y;
				if(sqrtf(dx * dx + dy * dy) > half_size) {
					continue;
				}
			}
			dx = xorshift32_n(&game->rand_state, scatter + 1);
			dy = xorshift32_n(&game->rand_state, scatter + 1);
			game->grid[GRID_GET_I(clamp_grid_w(x0 + dx), clamp_grid_h(y0 + dy))] = material;
			//printf("x:%d y:%d", clamp_grid_w(x0 + dx), clamp_grid_h(y0 + dy));
		}
	}
#endif
}

void game_tick(fsgame_t* game) {
	material_type_e_t mat_type;
	material_t mat;
	int index, new_index;
	bitset_clear(game->updated_cells_bitset, BITSET_SIZE_ARRAY(GRID_SIZE));
	for(int i = 0; i < GRID_SIZE; i++) {
		index = game->index_shuffle[i];
		mat_type = game->grid[index];
		mat = game->materials[mat_type];

		if(mat_type == AIR || bitset_get(game->updated_cells_bitset, index) || outof_grid(index, mat.speed, mat.direction) ||
				(mat.death_chance != 0 && (/*material.death_chance == DEATH_CHANCE_MAX ||*/ mat.death_chance > xorshift32_n(&game->rand_state, DEATH_CHANCE_MAX)))) {
			continue;
		}

		new_index = index 
			+ ((mat.direction & DOWN) != 0) * mat.speed * GRID_WIDTH
			- ((mat.direction & UP) != 0) * mat.speed * GRID_WIDTH
			+ ((mat.direction & RIGHT) != 0) * mat.speed
			- ((mat.direction & LEFT) != 0) * mat.speed;

		if(game->grid[new_index] != AIR) {
			const int delta = -mat.speed + 2 * xorshift32_n(&game->rand_state, mat.speed * 2);
			const int grid_x = new_index % GRID_WIDTH;

			int new_grid_x = clamp_grid_w(grid_x + delta);
			if(game->grid[new_index - grid_x + new_grid_x] == AIR) {
				new_index = new_index - grid_x + new_grid_x;
			}else {
				new_grid_x = clamp_grid_w(grid_x - delta);
				if(game->grid[new_index - grid_x + new_grid_x] == AIR) {
					new_index = new_index - grid_x + new_grid_x;
				}
			}
		}

		if(game->grid[new_index] == AIR) {
			game->grid[index] = AIR;
			game->grid[new_index] = mat_type;
			bitset_set_weak(game->updated_cells_bitset, new_index, mat_type != AIR);
		}

	}

	//printf("\n");
}

