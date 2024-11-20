#include "simulation.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../include/bitset.h"

#define LIQUID_SIDE_FLOW_MULTIPLIER 5

//why i have this? because
#ifdef NDEBUG
# define NODEFAULT __assume(0)
#else
# define NODEFAULT assert(0)
#endif

simulation_t* simulation_new(arena_t* a) {
	simulation_t* sim = arena_alloc(a, sizeof(simulation_t));
	sim->arena = a;
	return sim;
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
static coll_res_typeonly_t material_get_collision(material_t* from, material_t* to) {
	if(from->flaming && to->flamable || from->melting && to->meltable) {
		return COLL_REPLACE;
	}
	if(from->mass > to->mass) {
		return COLL_SWAP;
	}
	return COLL_CANT_MOVE;
}
static force_inline int rand_sign(xorshift32_state* rand_state) {
	int rand_val = xorshift32(rand_state);
	return (rand_val & 1) - (~rand_val & 1);
}
static interaction_t WALL_INTERACTION = { COLL_CANT_MOVE, 1.f, 0.f };
static void precompute_collisions(simulation_t* sim) {
	material_t* materials = sim->materials;
	material_t* mat_from, *mat_to;
	for(int mat_type_from = 0; mat_type_from < MATERIALS_COUNT; mat_type_from++) {
		for(int mat_type_to = 0; mat_type_to < MATERIALS_COUNT; mat_type_to++) {
			mat_from = materials + mat_type_from;
			mat_to = materials + mat_type_to;
			interaction_t* interaction = sim->interactions + INTERACTION_GET_I(mat_type_from, mat_type_to);
			interaction->collision = material_get_collision(mat_from, mat_to);
			interaction->bounce_deviation = ((mat_from->roundness + mat_to->roundness) * 0.5f) * BOUNCE_DEVIATION_MAX;
			interaction->bounce = mat_from->bounciness * mat_to->bounciness; // * (1.f + interaction->bounce_deviation * rand_sign(&sim->rand_state));
		}
	}
}
void simulation_init(simulation_t* sim) {
	sim->rand_state.a = time(NULL);

	sim->gravity = 0.1f;
	sim->air_drag = 0.9f;
	sim->terminal_v = 1.f;

	memset(sim->grid, 0, GRID_SIZE * sizeof(material_type_e_t));

	for(int i = 0; i < GRID_CELLS_SIZE; i++) {
		sim->acceleration_map[i].x = 0.0f;
		sim->acceleration_map[i].y = 0.0f;
	}

	for(int i = 0; i < GRID_SIZE; i++) {
		sim->index_shuffle[i] = i;
	}
	array_shuffle(sim->index_shuffle, GRID_SIZE, &sim->rand_state);

	bitset_clear(sim->updated_cells_bitset, GRID_SIZE);

	init_get_materials(sim);
	precompute_collisions(sim);
}

static force_inline bool outof_grid(int index) {
	return index < 0 || index >= GRID_SIZE;
}
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
void simulation_replace_cell(simulation_t* sim, int x, int y, material_type_e_t material) {
	int index = GRID_GET_I(x, y);
	material_t mat = sim->materials[material];
	pdata_t* pdata = sim->particles + index;
	if(material == sim->grid[index]) {
		return;
	}
	sim->grid[index] = material;
	pdata->x = x + 0.5f;
	pdata->y = y + 0.5f;
	pdata->vx = 0.f;
	pdata->vy = 0.f;
	pdata->t = mat.aim_temp;
}
void simulation_place(simulation_t* sim, material_type_e_t material, int x, int y, int size, int scatter, bool round) {
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
			dx = xorshift32_n(&sim->rand_state, scatter + 1);
			dy = xorshift32_n(&sim->rand_state, scatter + 1);
			sim->grid[GRID_GET_I(x0 + dx, y0 + dy)] = material;
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
			sx = -half_scatter + xorshift32_n(&sim->rand_state, scatter + 1);
			sy = -half_scatter + xorshift32_n(&sim->rand_state, scatter + 1);
			if(round){
				dx = x0 - x;
				dy = y0 - y;
				dx += sx;
				dy += sy;
				if(sqrtf(dx * dx + dy * dy) > half_size + half_scatter) {
					continue;
				}
			}
			simulation_replace_cell(sim, clamp_grid_w(x0 + sx), clamp_grid_h(y0 + sy), material);
			//printf("x:%d y:%d", clamp_grid_w(x0 + dx), clamp_grid_h(y0 + dy));
		}
	}
#endif
}

static force_inline coll_res_typeonly_t can_move(simulation_t* sim, material_type_e_t mat_from, int x_to, int y_to) {
	if(outof_grid_xy(x_to, y_to)) {
		return COLL_CANT_MOVE;
	}
	return sim->interactions[INTERACTION_GET_I(mat_from, sim->grid[GRID_GET_I(x_to, y_to)])].collision;
}
//static force_inline bool cell_occupied(simulation_t* sim, int x, int y) {
//	if(outof_grid_xy(x, y)) {
//		return true;
//	}
//	return sim->grid[GRID_GET_I(x, y)] != MAT_AIR;
//}
//#define ALWAYS_SCALE_MOVE
static force_inline coll_res_full_t clamp_move(simulation_t* sim, material_type_e_t mat_type, float x_from, float y_from, float x_to, float y_to, float* x_res, float* y_res) {
	if((int)x_from == (int)x_to && (int)y_from == (int)y_to) {
		*x_res = x_to;
		*y_res = y_to;
		return COLL_CANT_MOVE;
	}
	coll_res_full_t res;
	float delta_y = y_to - y_from;
	float delta_x = x_to - x_from;
	//printf("%f-%f ", delta_x, delta_y);

	float dir_y, dir_x;
	float abs_dy = fabsf(delta_y);
	float abs_dx = fabsf(delta_x);
#ifdef ALWAYS_SCALE_MOVE
	//float abs_y = fabsf(delta_y);
	//float abs_x = fabsf(delta_x);
	//if(abs_y >= abs_x) {
	//	dir_y = signf(delta_y);
	//	//dir_y = clampf(delta_y, -1.f, 1.f);
	//	dir_x = delta_x / abs_y;
	//}else {
	//	dir_y = delta_y / abs_x;
	//	dir_x = signf(delta_x);
	//	//dir_x = clampf(delta_x, -1.f, 1.f);
	//}
	float inv_dmax = 1.f / fmaxf(abs_dy, abs_dx);
	dir_y = delta_y * inv_dmax;
	dir_x = delta_x * inv_dmax;
#else
	if(abs_dy > 1.f && abs_dx > 1.f) {
		float inv_dmax = 1.f / fmaxf(abs_dy, abs_dx);
		dir_y = delta_y * inv_dmax;
		dir_x = delta_x * inv_dmax;
	}else {
		dir_y = clampf(delta_y, -1.f, 1.f);
		dir_x = clampf(delta_x, -1.f, 1.f);
	}
#endif

	do {
		x_from += dir_x;
		y_from += dir_y;
		res = can_move(sim, mat_type, (int)x_from, (int)y_from);
		if(res == COLL_CANT_MOVE) {
			if((int)(x_from - dir_x) != (int)x_from) {
				if((int)(x_from - dir_x) < (int)x_from) {
					res |= COLL_FLAG_RIGHT;
				}else {
					res |= COLL_FLAG_LEFT;
				}
			}
			if((int)(y_from - dir_y) != (int)y_from) {
				if((int)(y_from - dir_y) < (int)y_from) {
					res |= COLL_FLAG_DOWN;
				}else {
					res |= COLL_FLAG_UP;
				}
			}
			break;
		}else {
			*x_res = x_from;
			*y_res = y_from;
			if(res == COLL_REPLACE) {
				break;
			}
		}
	} while((int)y_from != (int)y_to && (int)x_from != (int)x_to);
	return res;
}
static force_inline void simulation_swap(simulation_t* sim, int index1, int index2) {
	//printf("%f %f %f %f\n", sim->particles[index1].x, sim->particles[index1].y, sim->particles[index2].x, sim->particles[index2].y);
	SWAP(material_type_e_t, sim->grid[index1], sim->grid[index2]);
	SWAP(pdata_t, sim->particles[index1], sim->particles[index2]);
}
static void handle_cell(simulation_t* sim, int index/*, int x, int y*/) {
	material_type_e_t* grid = sim->grid;
	material_type_e_t mat_type = grid[index];
	material_t* materials = sim->materials;
	material_t* mat = materials + mat_type;
	pdata_t* pdata = sim->particles + index;

	int x = (int)pdata->x;
	int y = (int)pdata->y;

	float terminal_v = sim->terminal_v;
	xy_t acceleration = sim->acceleration_map[DATA_MAP_GET_I(x, y)];
	pdata->vx = clampf((pdata->vx + acceleration.x), -terminal_v, terminal_v) * sim->air_drag;
	pdata->vy = clampf((pdata->vy + acceleration.y + sim->gravity), -terminal_v, terminal_v) * sim->air_drag;

	coll_res_full_t coll = clamp_move(sim, mat_type, pdata->x, pdata->y, pdata->x + pdata->vx, pdata->y + pdata->vy, &pdata->x, &pdata->y);
	//TODO: coll
	
	int new_x = (int)pdata->x;
	int new_y = (int)pdata->y;
	if(new_x == x && new_y == y) {
		if(mat->tick) {
			mat->tick(sim, index, x, y);
		}
		return;
	}

	interaction_t* interaction;
	if(new_x > x && can_move(sim, mat_type, new_x + 1, new_y) == COLL_CANT_MOVE) {
		switch(coll & COLL_TYPE_MASK) {
		case COLL_CANT_MOVE:
			pdata->x = (float)new_x + 0.5f;
			break;
		case COLL_REPLACE:
			pdata->x = (float)new_x + 0.5f;
			pdata->vx = 0;
			break;
		case COLL_SWAP:
			if(new_x + 1 >= GRID_WIDTH) {
				interaction = &WALL_INTERACTION;
			}else {
				interaction = sim->interactions + INTERACTION_GET_I(mat_type, grid[GRID_GET_I(new_x + 1, new_y)]);
			}
			pdata->vx = -pdata->vx * interaction->bounce;
			float dev = 1.f + interaction->bounce_deviation * rand_sign(&sim->rand_state);
			pdata->vy = pdata->vy * dev;
			break;
		default: NODEFAULT;
		}
	}else if(new_x < x && can_move(sim, mat_type, new_x - 1, new_y) == COLL_CANT_MOVE) {
		switch(coll & COLL_TYPE_MASK) {
		case COLL_CANT_MOVE:
			pdata->x = (float)new_x + 0.5f;
			break;
		case COLL_REPLACE:
			pdata->x = (float)new_x + 0.5f;
			pdata->vx = 0;
			break;
		case COLL_SWAP:
			if(new_x - 1 < 0) {
				interaction = &WALL_INTERACTION;
			}else {
				interaction = sim->interactions + INTERACTION_GET_I(mat_type, grid[GRID_GET_I(new_x - 1, new_y)]);
			}
			pdata->vx = -pdata->vx * interaction->bounce;
			float dev = 1.f + interaction->bounce_deviation * rand_sign(&sim->rand_state);
			pdata->vy = pdata->vy * dev;
			break;
		default: NODEFAULT;
		}
	}
	if(new_y > y && can_move(sim, mat_type, new_x, new_y + 1) == COLL_CANT_MOVE) {
		switch(coll & COLL_TYPE_MASK) {
		case COLL_CANT_MOVE:
			pdata->y = (float)new_y + 0.5f;
			break;
		case COLL_REPLACE:
			pdata->y = (float)new_y + 0.5f;
			pdata->vy = 0;
			break;
		case COLL_SWAP:
			if(new_y + 1 >= GRID_HEIGHT) {
				interaction = &WALL_INTERACTION;
			}else {
				interaction = sim->interactions + INTERACTION_GET_I(mat_type, grid[GRID_GET_I(new_x, new_y + 1)]);
			}
			pdata->vy = -pdata->vy * interaction->bounce;
			float dev = 1.f + interaction->bounce_deviation * rand_sign(&sim->rand_state);
			pdata->vx = pdata->vx * dev;
			break;
		default: NODEFAULT;
		}
	}else if(new_y < y && can_move(sim, mat_type, new_y, new_y - 1) == COLL_CANT_MOVE) {
		switch(coll & COLL_TYPE_MASK) {
		case COLL_CANT_MOVE:
			pdata->y = (float)new_y + 0.5f;
			break;
		case COLL_REPLACE:
			pdata->y = (float)new_y + 0.5f;
			pdata->vy = 0;
			break;
		case COLL_SWAP:
			if(new_y - 1 < 0) {
				interaction = &WALL_INTERACTION;
			}else {
				interaction = sim->interactions + INTERACTION_GET_I(mat_type, grid[GRID_GET_I(new_x, new_y - 1)]);
			}
			pdata->vy = -pdata->vy * interaction->bounce;
			float dev = 1.f + interaction->bounce_deviation * rand_sign(&sim->rand_state);
			pdata->vx = pdata->vx * dev;
			break;
		default: NODEFAULT;
		}
	}

	if(mat->tick) {
		mat->tick(sim, index, new_x, new_y);
	}

	int new_index = GRID_GET_I(new_x, new_y);
	simulation_swap(sim, index, new_index);
}

//static force_inline void handle_pdata(simulation_t* sim, int index) {
//	pdata_t* pdata = sim->particles + index;
//	float max_v = sim->max_v;
//	pdata->vx = clampf(pdata->vx * sim->air_drag, -max_v, max_v);
//	pdata->vy = clampf((pdata->vy + sim->gravity) * sim->air_drag, -max_v, max_v);
//}
static void process_cell(simulation_t* sim, int index/*, int x, int y*/) {
	//printf("\npre: %d %d\n", x, y);
	//handle_pdata(sim, index);
	//printf("x: %d, y: %d, nx: %d, ny: %d\n", x, y, new_x, new_y);
	handle_cell(sim, index/*, x, y*/);
}

void simulation_tick(simulation_t* sim) {
	material_type_e_t mat_type;
	material_t* mat;
	material_type_e_t* grid = sim->grid;
	material_t* materials = sim->materials;
	pdata_t* pdata;
	int* index_shuffle = sim->index_shuffle;
	int index, x, y;

	bitset_clear(sim->updated_cells_bitset, BITSET_SIZE_ARRAY(GRID_SIZE));
	for(int y_ = 0; y_ < GRID_HEIGHT; y_++) {
		for(int x_ = 0; x_ < GRID_WIDTH; x_++) {
			index = index_shuffle[GRID_GET_I(x_, y_)];
			mat_type = grid[index];
			mat = materials + mat_type;

			if(mat_type == MAT_AIR || bitset_get(sim->updated_cells_bitset, index)) {
				continue;
			}
			if(mat->death_chance != 0 && (/*material.death_chance == DEATH_CHANCE_MAX ||*/ mat->death_chance > xorshift32_n(&sim->rand_state, DEATH_CHANCE_MAX))) {
				pdata = sim->particles + index;
				simulation_replace_cell(sim, pdata->x, pdata->y, MAT_AIR);
				bitset_set_weak(sim->updated_cells_bitset, index, mat_type != MAT_AIR);
				continue;
			}

			//GRID_INDEX_GET_XY(index, x, y);
			process_cell(sim, index/*, x, y*/);
		}
	}
	printf("\n");
}

void simulation_free(simulation_t* sim) {
	//arena_free(sim->arena);
}