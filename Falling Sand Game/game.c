#include "game.h"

#include <time.h>
#include <stdlib.h>


fsgame_t* new_game(arena_t* a) {
	fsgame_t* game = arena_alloc(a, sizeof(fsgame_t));
	game->arena = a;
	return game;
}

#define SET_MATERIAL_DEFAULT(mat) \
	(mat).direction = 0; \
	(mat).speed = 0; \
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
	mat->density = DENSITY_MAX;

	mat = &game->materials[WATER];
	SET_MATERIAL_DEFAULT_NONSOLID(*mat);
	mat->direction |= DOWN;
	mat->speed = 1;
	mat->density = DENSITY_MAX * 0.3;
	
	mat = &game->materials[SAND];
	SET_MATERIAL_DEFAULT_SOLID(*mat);
	mat->direction |= DOWN;
	mat->speed = 1;
	mat->density = DENSITY_MAX * 0.5;

	mat = &game->materials[ROCK];
	SET_MATERIAL_DEFAULT_SOLID(*mat);
	mat->density = DENSITY_MAX * 0.65;

	mat = &game->materials[WOOD];
	SET_MATERIAL_DEFAULT_SOLID(*mat);
	mat->density = DENSITY_MAX * 0.5;
	mat->flamable = true;

	mat = &game->materials[FIRE];
	SET_MATERIAL_DEFAULT_NONSOLID(*mat);
	mat->direction |= UP;
	mat->speed = 1;
	mat->flaming = true;
}
void init_game(fsgame_t* game) {
	srand(time(NULL));
	game->rand_state.a = rand();

	init_materials(game);
}

#define ARRAY2D_GET(x, y, ROW_WIDTH) ((y) * (ROW_WIDTH) + (x))

static inline bool outof_grid(int index, uint8_t speed, uint8_t direction) {
	return (direction & RIGHT && (index % GRID_WIDTH + speed) >= GRID_WIDTH) ||
		   (direction & LEFT  && (index % GRID_WIDTH - speed) < 0) ||
		   (direction & DOWN  && (index + speed * GRID_WIDTH) >= GRID_SIZE) ||
		   (direction & UP    && (index - speed * GRID_WIDTH) < 0);
}

void tick(fsgame_t* game) {
	material_type_e_t mat_type;
	material_t mat;
	int new_cell_pos = 0;
	for(int i = 0;i < GRID_SIZE; i++) {
		mat_type = game->grid[i];
		mat = game->materials[mat_type];

		if(mat_type == AIR || outof_grid(i, mat.speed, mat.direction) ||
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
		}else {
			
		}

	}
}
