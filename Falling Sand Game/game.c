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
void game_init(fsgame_t* game) {
	game->rand_state.a = time(NULL);

	memset(game->grid, 0, GRID_SIZE * sizeof(material_type_e_t));

	init_materials(game);
}

#define GRID_GET_I(x, y) ((y) * (GRID_WIDTH) + (x))

static inline bool outof_grid(int index, uint8_t speed, uint8_t direction) {
	return (direction & RIGHT && (index % GRID_WIDTH + speed) >= GRID_WIDTH) ||
		   (direction & LEFT  && (index % GRID_WIDTH - speed) < 0) ||
		   (direction & DOWN  && (index + speed * GRID_WIDTH) >= GRID_SIZE) ||
		   (direction & UP    && (index - speed * GRID_WIDTH) < 0);
}

int clamp(int val, int min, int max) {
	if(val < min) {
		return min;
	}
	if(val > max) {
		return max;
	}
	return val;
}

void game_place(fsgame_t* game, material_type_e_t material, int x, int y, int size, int scatter, bool round) {
#ifdef GRID_PLACE_PRE_CLAMP
	int half_size = size / 2;
	int x_to = clamp(x + half_size, 0, GRID_WIDTH);
	int y_to = clamp(y + half_size, 0, GRID_HEIGHT);
	int dx, dy;
	for(int y0 = clamp(y - half_size, 0, GRID_HEIGHT); y0 < y_to; y0++) {
		for(int x0 = clamp(x - half_size, 0, GRID_WIDTH); x0 < x_to; x0++) {
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
			game->grid[GRID_GET_I(clamp(x0 + dx, 0, GRID_WIDTH), clamp(y0 + dy, 0, GRID_HEIGHT))] = material;
		}
	}
#endif
}

void game_tick(fsgame_t* game) {
	material_type_e_t mat_type;
	material_t mat;
	int new_cell_pos = 0;
	size_t* bitset = bitset_new_size(GRID_SIZE);
	for(int i = 0;i < GRID_SIZE; i++) {
		mat_type = game->grid[i];
		mat = game->materials[mat_type];

		if(mat_type == AIR || bitset_get(bitset, i) || outof_grid(i, mat.speed, mat.direction) ||
				mat.death_chance != 0 && (/*material.death_chance == DEATH_CHANCE_MAX ||*/ mat.death_chance > xorshift32_n(&game->rand_state, DEATH_CHANCE_MAX))) {
			continue;
		}

		game->grid[i] = AIR;

		new_cell_pos = i 
			+ ((mat.direction & DOWN) != 0) * mat.speed * GRID_WIDTH
			- ((mat.direction & UP) != 0) * mat.speed * GRID_WIDTH
			+ ((mat.direction & RIGHT) != 0) * mat.speed
			- ((mat.direction & LEFT) != 0) * mat.speed;

		if(game->grid[new_cell_pos] == AIR) {
			game->grid[new_cell_pos] = mat_type;
			bitset_set_weak(bitset, new_cell_pos, mat_type != AIR);
		}else {
			new_cell_pos = i;
			game->grid[new_cell_pos] = mat_type;
			bitset_set_weak(bitset, new_cell_pos, mat_type != AIR);
		}

	}
}

