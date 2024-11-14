#include "game.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../include/bitset.h"

#define LIQUID_SIDE_FLOW_MULTIPLIER 5


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
	(mat).viscosity = 0; \
	(mat).solid = false; \
	(mat).flamable = false; \
	(mat).flaming = false

#define SET_MATERIAL_DEFAULT_NONSOLID(mat) \
	SET_MATERIAL_DEFAULT(mat); \
	(mat).solid = false

#define SET_MATERIAL_DEFAULT_SOLID(mat) \
	SET_MATERIAL_DEFAULT(mat); \
	(mat).solid = true

static void materails_review_viscosity(fsgame_t* game) { //set viscosity of 0 to 1
	for(int i = 0; i < MATERIALS_COUNT; i++) {
		if(game->materials[i].viscosity == 0.f) {
			game->materials[i].viscosity = 1.f;
		}
	}
}
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
	mat->direction |= SIDES;
	mat->speed = 3;
	mat->viscosity = 0.35f;
	mat->density = DENSITY_MAX * 0.3;

	mat = &game->materials[SAND];
	SET_MATERIAL_DEFAULT_SOLID(*mat);
	mat->color = (rgba_t){250, 242, 197, 255};
	mat->direction |= DOWN;
	mat->speed = 1;
	mat->density = DENSITY_MAX * 0.5;

	mat = &game->materials[ROCK];
	SET_MATERIAL_DEFAULT_SOLID(*mat);
	mat->color = (rgba_t){113, 103, 100, 255};
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
	mat->density = DENSITY_MAX * 0.1;
	mat->death_chance = 20;
	mat->speed = 1;
	mat->flaming = true;

	materails_review_viscosity(game);
}

#define SWAP(T, a, b) \
	do { \
		T tmp = (a); \
		(a) = (b); \
		(b) = tmp; \
	} while(0)

static void array_shuffle(int* arr, uint32_t size, xorshift32_state* rand_state) {
	for (uint32_t i = size - 1; i > 0; i--) {
		uint32_t j = xorshift32_n(rand_state, i + 1);
		SWAP(int, arr[i], arr[j]);
	}
}
void game_init(fsgame_t* game) {
	game->rand_state.a = time(NULL);

	game->gravity = 1.f;

	memset(game->grid, 0, GRID_SIZE * sizeof(material_type_e_t));
	memset(game->temperatures, 0, GRID_SIZE * sizeof(temperature_t));
	memset(game->flags, 0, GRID_SIZE * sizeof(material_flags_e_t));

	for(int i = 0; i < GRID_SIZE; i++) {
		game->index_shuffle[i] = i;
	}
	array_shuffle(game->index_shuffle, GRID_SIZE, &game->rand_state);

	//bitset_clear(game->updated_cells_bitset, GRID_SIZE);


	init_materials(game);
}

#define GRID_GET_I(x, y) ((y) * (GRID_WIDTH) + (x))

static force_inline bool outof_grid(int index) {
	return index < 0 || index >= GRID_SIZE;
}
//static inline bool outof_grid_move(int index, uint8_t speed, uint8_t direction) {
//	return (direction & SIDES && (index % GRID_WIDTH + speed) >= GRID_WIDTH) ||
//		   (direction & SIDES && (index % GRID_WIDTH - speed) < 0) ||
//		   (direction & DOWN  && (index + speed * GRID_WIDTH) >= GRID_SIZE) ||
//		   (direction & UP    && (index - speed * GRID_WIDTH) < 0);
//}
static force_inline bool outof_grid_x(int x) {
	return x < 0 || x >= GRID_WIDTH;
}
static force_inline bool outof_grid_y(int y) {
	return y < 0 || y >= GRID_HEIGHT;
}
static force_inline bool outof_grid_xy(int x, int y) {
	return x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT;
}

static force_inline int clamp_grid_w(int val) {
	return clampi(val, 0, GRID_WIDTH - 1);
}
static force_inline int clamp_grid_h(int val) {
	return clampi(val, 0, GRID_HEIGHT - 1);
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
	const int half_scatter = (scatter + 1) / 2;
	const int half_size = size / 2;
	const int x_to = x + half_size + (size & 1);
	const int y_to = y + half_size + (size & 1);
	int dx, dy, sx, sy;
	for(int y0 = y - half_size; y0 < y_to; y0++) {
		for(int x0 = x - half_size; x0 < x_to; x0++) {
			sx = -half_scatter + xorshift32_n(&game->rand_state, scatter + 1);
			sy = -half_scatter + xorshift32_n(&game->rand_state, scatter + 1);
			if(round){
				dx = x0 - x;
				dy = y0 - y;
				dx += sx;
				dy += sy;
				if(sqrtf(dx * dx + dy * dy) > half_size + half_scatter) {
					continue;
				}
			}
			game->grid[GRID_GET_I(clamp_grid_w(x0 + sx), clamp_grid_h(y0 + sy))] = material;
			//printf("x:%d y:%d", clamp_grid_w(x0 + dx), clamp_grid_h(y0 + dy));
		}
	}
#endif
}

typedef enum {
	CANT,
	POSSIBLE,
	REPLACE,
} collision_res_e_t;
static force_inline collision_res_e_t material_handle_collision(material_t* from, material_t* to) {
	if(from->flaming && to->flaming) {
		return REPLACE;
	}
	if(from->density < to->density) {
		return POSSIBLE;
	}
	return CANT;
}
static int game_get_cell_new_pos(fsgame_t* game, int index) {
	material_type_e_t* grid = game->grid;
	material_t* materials = game->materials;
	material_type_e_t mat_type = grid[index];
	material_t* mat = materials + mat_type;

	if(mat->speed <= 0){
		return index;
	}

	bool sides = mat->direction & SIDES;
	int y = index / GRID_WIDTH;
	int x = index - GRID_WIDTH * y;
	int inv_viscosity = 1.f / mat->viscosity;
	int direction_y = 1 * ((mat->direction & DOWN) != 0) - 1 * ((mat->direction & UP) != 0);
	int delta_y = direction_y * (xorshift32_n(&game->rand_state, mat->speed) + 1);

	int cnt_x = 0;
	int new_x = x;
	int new_y = y;

	//new_y = y + direction_y;
	//bool can_move = false;
	//do {
	//	if(outof_grid_y(new_y) || material_handle_collision(materials + grid[GRID_GET_I(x, new_y)], mat) == CANT) {
	//		break;
	//	}
	//	can_move = true;
	//	new_y += direction_y;
	//} while(true);
	//if(can_move) {
	//	return GRID_GET_I(x, new_y - direction_y);
	//}
	//int delta_x = -mat->speed + xorshift32_n(&game->rand_state, mat->speed * 2);
	//delta_x += delta_x >= 0;
	//delta_x *= inv_viscosity;

	int delta_x = (-mat->speed + xorshift32_n(&game->rand_state, mat->speed * 2 + 1)) * inv_viscosity;
	int direction_x = sign(delta_x);
	//if(!sides) {
	//	delta_x = direction_x;
	//}

	int end_x = x + delta_x + direction_x;
	int end_y = y + delta_y + direction_y;
	while(!outof_grid_x(new_x)) {
		new_y = y + !sides * direction_y;
		bool can_move = false;
		while(!outof_grid_y(new_y)) {
			if(material_handle_collision(materials + grid[GRID_GET_I(new_x, new_y)], mat) != CANT){
				can_move = true;
			}else if(new_y != y || new_x != x){ //break if not initial cell
				break;
			}
			new_y += direction_y;
			if(new_y == end_y) {
				//if(new_x == x && new_y - direction_y == y + direction_y) {
				//	delta_y *= inv_viscosity;
				//	end_y = y + delta_y + direction_y;
				//}
				break;
			}
		}
		new_y -= direction_y;
		if(can_move) {
			return GRID_GET_I(new_x, new_y);
		}
		new_x += direction_x;
		if(new_x == end_x || (outof_grid_xy(new_x, new_y) || material_handle_collision(materials + grid[GRID_GET_I(new_x, new_y)], mat) == CANT)){
			if(cnt_x++ >= abs(direction_x)) {
				break;
			}
			direction_x = -direction_x;
			delta_x  = -delta_x;
			new_x = x + direction_x;
			end_x = x + delta_x + direction_x;
		}
	}
	return index;
}
force_inline void game_swap(fsgame_t* game, int index1, int index2) {
	SWAP(material_type_e_t, game->grid[index1], game->grid[index2]);
}
void game_handle_solid(fsgame_t* game, int x, int y) {
	const int index = GRID_GET_I(x, y);
	const material_type_e_t* grid = game->grid;
	const material_t* materials = game->materials;
	material_type_e_t mat_type = grid[index];
	material_t mat = materials[mat_type];

	int weight = game->gravity * mat.density;
	if(weight == 0) {
		return;
	}
	weight += sign(weight);
	int delta_y = xorshift32_n(&game->rand_state, weight) + 1;
	int target_y = clamp_grid_h(y + delta_y);
	int direction_y = sign(delta_y);

	int new_x, new_y;
	bool can_place = false;
	if(y != target_y){
		new_y = y + direction_y;
		do {
			if(materials[grid[GRID_GET_I(x, new_y)]].density < mat.density) {
				can_place = true;
				if(new_y == target_y) {
					break;
				}
				new_y += direction_y;
			} else {
				break;
			}
		} while(true);
		if(can_place) {
			game_swap(game, index, GRID_GET_I(x, new_y));
			return;
		}
	}

	int delta_x = (-weight + xorshift32_n(&game->rand_state, weight * 2) * sign(weight)) / 2;
	delta_x += delta_x >= 0; //from -weight/2 to weight/2, excluding 0
	int target_x = clamp_grid_w(x + delta_x);
	int direction_x = sign(delta_x);
	for(int i = 0; i < 2; i++){
		if(x != target_x){
			can_place = false;
			new_x = x + direction_x;
			do {
				if(materials[grid[GRID_GET_I(new_x, y)]].density < mat.density) {
					can_place = true;
					if(new_x == target_x) {
						break;
					}
					new_x += direction_x;
				} else {
					break;
				}
			} while(true);
			if(can_place) {
				new_y = y;
				do {
					if(materials[grid[GRID_GET_I(new_x, new_y)]].density < mat.density) {
						if(new_y == target_y) {
							break;
						}
						new_y += direction_y;
					} else {
						break;
					}
				} while(true);
				game_swap(game, index, GRID_GET_I(new_x, new_y));
			}
		}
		delta_x = -delta_x;
		target_x = clamp_grid_w(x + delta_x);
		direction_x = -direction_x;
	}
}

void game_tick(fsgame_t* game) {
	material_type_e_t mat_type;
	material_t* mat;
	material_type_e_t* grid = game->grid;
	material_t* materials = game->materials;
	int index, new_index;
	bitset_clear(game->updated_cells_bitset, BITSET_SIZE_ARRAY(GRID_SIZE));
	for(int i = 0; i < GRID_SIZE; i++) {
		index = game->index_shuffle[i];
		mat_type = grid[index];
		mat = materials + mat_type;

		if(mat_type == AIR || bitset_get(game->updated_cells_bitset, index)) {
			continue;
		}
		if(mat->death_chance != 0 && (/*material.death_chance == DEATH_CHANCE_MAX ||*/ mat->death_chance > xorshift32_n(&game->rand_state, DEATH_CHANCE_MAX))) {
			game->grid[index] = AIR;
			bitset_set_weak(game->updated_cells_bitset, index, mat_type != AIR);
			continue;
		}

		new_index = game_get_cell_new_pos(game, index);
		if(!outof_grid(new_index) && material_handle_collision(materials + grid[new_index], mat) != CANT) {
			if(xorshift32_n(&game->rand_state, mat->density) > materials[grid[new_index]].density * materials[grid[new_index]].viscosity){
				SWAP(material_type_e_t, grid[index], grid[new_index]);
				bitset_set_weak(game->updated_cells_bitset, new_index, mat_type != AIR);
			}
		}
	}

	//printf("\n");
}

