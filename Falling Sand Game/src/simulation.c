#include "simulation.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../include/bitset.h"

#define LIQUID_SIDE_FLOW_MULTIPLIER 5


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
static collision_res_e_t material_get_collision(material_t* from, material_t* to) {
	if(from->flaming && to->flaming) {
		return COLL_REPLACE;
	}
	if(from->mass < to->mass) {
		return COLL_SWAP;
	}
	return COLL_CANT_MOVE;
}
static void precompute_collisions(simulation_t* sim) {
	material_t* materials = sim->materials;
	material_t* mat_from, *mat_to;
	for(int mat_type_from = 0; mat_type_from < MATERIALS_COUNT; mat_type_from++) {
		for(int mat_type_to = 0; mat_type_to < MATERIALS_COUNT; mat_type_to++) {
			mat_from = materials + mat_type_from;
			mat_to = materials + mat_type_to;
			sim->interactions[INTERACTION_GET_I(mat_type_from, mat_type_to)].collision = material_get_collision(mat_from, mat_to);
			sim->interactions[INTERACTION_GET_I(mat_type_from, mat_type_to)].bounce = mat_from->bounciness * mat_to->bounciness;
		}
	}
}
void simulation_init(simulation_t* sim) {
	sim->rand_state.a = time(NULL);

	//sim->gravity = 0.01f;
	sim->gravity = 1.f;
	sim->air_drag = 0.95f;

	memset(sim->grid, 0, GRID_SIZE * sizeof(material_type_e_t));

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
	sim->grid[index] = material;
	pdata_t* pdata = sim->particles + index;
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

static force_inline collision_res_e_t can_move(simulation_t* sim, material_type_e_t mat_from, int x_to, int y_to) {
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
static force_inline collision_res_e_t clamp_move(simulation_t* sim, material_type_e_t mat_type, float x_from, float y_from, float x_to, float y_to, int* x_res, int* y_res) {
	if((int)x_from == (int)x_to && (int)y_from == (int)y_to) {
		return COLL_CANT_MOVE;
	}
	collision_res_e_t res;
	float delta_y = y_to - y_from;
	float delta_x = x_to - x_from;
	float dir_y, dir_x;
	//if((int)x_from == (int)x_to) {
	//	dir_y = signf(delta_y);
	//	do {
	//		y_from += dir_y;
	//		res = can_move(sim, mat_type, (int)x_from, (int)y_from);
	//		if(res == COLL_CANT_MOVE) {
	//			break;
	//		}else {
	//			*y_res = (int)y_from;
	//			if(res == COLL_REPLACE) {
	//				break;
	//			}
	//		}
	//	} while((int)y_from != (int)y_to);
	//	return res;
	//}
	//if((int)y_from == (int)y_to) {
	//	dir_x = signf(delta_x);
	//	do {
	//		x_from += dir_x;
	//		res = can_move(sim, mat_type, (int)x_from, (int)y_from);
	//		if(res == COLL_CANT_MOVE) {
	//			break;
	//		}else {
	//			*x_res = (int)x_from;
	//			if(res == COLL_REPLACE) {
	//				break;
	//			}
	//		}
	//	} while((int)x_from != (int)x_to);
	//	return res;
	//}

	float abs_y = fabsf(delta_y);
	float abs_x = fabsf(delta_x);
	if(abs_y >= abs_x) {
		dir_x = delta_x / abs_y;
		//dir_y = delta_y / abs_y;
		dir_y = signf(delta_y);
	}else {
		//dir_x = delta_x / abs_x;
		dir_x = signf(delta_x);
		dir_y = delta_y / abs_x;
	}
	do {
		x_from += dir_x;
		y_from += dir_y;
		res = can_move(sim, mat_type, (int)x_from, (int)y_from);
		if(res == COLL_CANT_MOVE) {
			break;
		}else {
			*x_res = (int)x_from;
			*y_res = (int)y_from;
			if(res == COLL_REPLACE) {
				break;
			}
		}
	} while((int)y_from != (int)y_to && (int)x_from != (int)x_to);
	return res;
}
static force_inline collision_res_e_t get_cell_new_pos(simulation_t* sim, int index, int x, int y, int* new_x, int* new_y) {
	material_type_e_t* grid = sim->grid;
	material_type_e_t mat_type = grid[index];
	pdata_t* pdata = sim->particles + index;

	int data_index = DATA_MAP_GET_I(x, y);

	float vx = pdata->vx + sim->velocity_map[data_index].x;
	float vy = pdata->vy + sim->velocity_map[data_index].y;

	//float new_x = pdata->x + vx;
	//float new_y = pdata->y + vy;

	//int new_x, new_y;
	collision_res_e_t res = clamp_move(sim, mat_type, x, y, pdata->x + vx, pdata->y + vy, new_x, new_y);

	return res;

	//if(mat->speed <= 0){
	//	return index;
	//}
	//
	//bool sides = mat->direction & SIDES;
	//int y = index / GRID_WIDTH;
	//int x = index - GRID_WIDTH * y;
	//int inv_viscosity = 1.f / mat->viscosity;
	//int direction_y = 1 * ((mat->direction & DOWN) != 0) - 1 * ((mat->direction & UP) != 0);
	//int delta_y = direction_y * (xorshift32_n(&game->rand_state, mat->speed) + 1);
	//
	//int cnt_x = 0;
	//int new_x = x;
	//int new_y = y;
	//
	////new_y = y + direction_y;
	////bool can_move = false;
	////do {
	////	if(outof_grid_y(new_y) || material_handle_collision(materials + grid[GRID_GET_I(x, new_y)], mat) == CANT) {
	////		break;
	////	}
	////	can_move = true;
	////	new_y += direction_y;
	////} while(true);
	////if(can_move) {
	////	return GRID_GET_I(x, new_y - direction_y);
	////}
	////int delta_x = -mat->speed + xorshift32_n(&game->rand_state, mat->speed * 2);
	////delta_x += delta_x >= 0;
	////delta_x *= inv_viscosity;
	//
	//int delta_x = (-mat->speed + xorshift32_n(&game->rand_state, mat->speed * 2 + 1)) * inv_viscosity;
	//int direction_x = sign(delta_x);
	////if(!sides) {
	////	delta_x = direction_x;
	////}
	//
	//int end_x = x + delta_x + direction_x;
	//int end_y = y + delta_y + direction_y;
	//while(!outof_grid_x(new_x)) {
	//	new_y = y + !sides * direction_y;
	//	bool can_move = false;
	//	while(!outof_grid_y(new_y)) {
	//		if(material_handle_collision(materials + grid[GRID_GET_I(new_x, new_y)], mat) != COLL_CANT_MOVE){
	//			can_move = true;
	//		}else if(new_y != y || new_x != x){ //break if not initial cell
	//			break;
	//		}
	//		new_y += direction_y;
	//		if(new_y == end_y) {
	//			//if(new_x == x && new_y - direction_y == y + direction_y) {
	//			//	delta_y *= inv_viscosity;
	//			//	end_y = y + delta_y + direction_y;
	//			//}
	//			break;
	//		}
	//	}
	//	new_y -= direction_y;
	//	if(can_move) {
	//		return GRID_GET_I(new_x, new_y);
	//	}
	//	new_x += direction_x;
	//	if(new_x == end_x || (outof_grid_xy(new_x, new_y) || material_handle_collision(materials + grid[GRID_GET_I(new_x, new_y)], mat) == COLL_CANT_MOVE)){
	//		if(cnt_x++ >= abs(direction_x)) {
	//			break;
	//		}
	//		direction_x = -direction_x;
	//		delta_x  = -delta_x;
	//		new_x = x + direction_x;
	//		end_x = x + delta_x + direction_x;
	//	}
	//}
	//return index;
}
static force_inline void simulation_swap(simulation_t* sim, int index1, int index2) {
	SWAP(material_type_e_t, sim->grid[index1], sim->grid[index2]);
	SWAP(pdata_t, sim->particles[index1], sim->particles[index2]);
}
static void handle_new_cell_pos(simulation_t* sim, int index, int x, int y, int new_x, int new_y) {
	material_type_e_t* grid = sim->grid;
	material_type_e_t mat_type = grid[index];
	collision_res_e_t collision = can_move(sim, mat_type, new_x, new_y);
	if(collision == COLL_CANT_MOVE) {
		return;
	}
	material_t* materials = sim->materials;
	material_t* mat = materials + mat_type;
	pdata_t* pdata = sim->particles + index;
	interaction_t* interactions = sim->interactions;

	if(new_x > x && can_move(sim, mat_type, new_x + 1, new_y) == COLL_CANT_MOVE) {
		pdata->x = (float)new_x + 0.5f;
		if(collision == COLL_REPLACE) {
			pdata->vx = 0;
		} else{
			pdata->vx = -pdata->vx * interactions[INTERACTION_GET_I(mat_type, grid[GRID_GET_I(new_x + 1, new_y)])].bounce;
		}
	}else if(new_x < x && can_move(sim, mat_type, new_x - 1, new_y) == COLL_CANT_MOVE) {
		pdata->x = (float)new_x + 0.5f;
		if(collision == COLL_REPLACE) {
			pdata->vx = 0;
		} else{
			pdata->vx = -pdata->vx * interactions[INTERACTION_GET_I(mat_type, grid[GRID_GET_I(new_x - 1, new_y)])].bounce;
		}
	}
	if(new_y > y && can_move(sim, mat_type, new_x, new_y + 1) == COLL_CANT_MOVE) {
		pdata->y = (float)new_y + 0.5f;
		if(collision == COLL_REPLACE) {
			pdata->vy = 0;
		} else{
			pdata->vy = -pdata->vy * interactions[INTERACTION_GET_I(mat_type, grid[GRID_GET_I(new_x, new_y + 1)])].bounce;
		}
	}else if(new_y < y && can_move(sim, mat_type, new_y, new_y - 1) == COLL_CANT_MOVE) {
		pdata->y = (float)new_y + 0.5f;
		if(collision == COLL_REPLACE) {
			pdata->vy = 0;
		} else{
			pdata->vy = -pdata->vy * interactions[INTERACTION_GET_I(mat_type, grid[GRID_GET_I(new_x, new_y - 1)])].bounce;
		}
	}

	if(mat->tick) {
		mat->tick(sim, index, new_x, new_y);
	}

	simulation_swap(sim, index, GRID_GET_I(new_x, new_y));

}

static force_inline void handle_pdata(simulation_t* sim, int index) {
	pdata_t* pdata = sim->particles + index;
	pdata->vy += sim->gravity;
	pdata->vx *= sim->air_drag;
	pdata->vy *= sim->air_drag;
}
static void process_cell(simulation_t* sim, int index, int x, int y) {
	int new_x, new_y;
	handle_pdata(sim, index);
	get_cell_new_pos(sim, index, x, y, &new_x, &new_y);
	handle_new_cell_pos(sim, index, x, y, new_x, new_y);
}

void simulation_tick(simulation_t* sim) {
	material_type_e_t mat_type;
	material_t* mat;
	material_type_e_t* grid = sim->grid;
	material_t* materials = sim->materials;
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
				sim->grid[index] = MAT_AIR;
				bitset_set_weak(sim->updated_cells_bitset, index, mat_type != MAT_AIR);
				continue;
			}

			GRID_INDEX_GET_XY(index, x, y);
			process_cell(sim, index, x, y);
		}
	}
	//for(int i = 0; i < GRID_SIZE; i++) {
	//	index = sim->index_shuffle[i];
	//	mat_type = grid[index];
	//	mat = materials + mat_type;
	//
	//	if(mat_type == MAT_AIR || bitset_get(sim->updated_cells_bitset, index)) {
	//		continue;
	//	}
	//	if(mat->death_chance != 0 && (/*material.death_chance == DEATH_CHANCE_MAX ||*/ mat->death_chance > xorshift32_n(&sim->rand_state, DEATH_CHANCE_MAX))) {
	//		sim->grid[index] = MAT_AIR;
	//		bitset_set_weak(sim->updated_cells_bitset, index, mat_type != MAT_AIR);
	//		continue;
	//	}
	//
	//	//new_index = simulation_get_cell_new_pos(sim, index);
	//	//if(!outof_grid(new_index) && material_handle_collision(materials + grid[new_index], mat) != COLL_CANT_MOVE) {
	//	//	if(xorshift32_n(&sim->rand_state, mat->mass) > materials[grid[new_index]].mass * materials[grid[new_index]].viscosity){
	//	//		simulation_swap(sim, index, new_index);
	//	//		//SWAP(material_type_e_t, grid[index], grid[new_index]);
	//	//		bitset_set_weak(sim->updated_cells_bitset, new_index, mat_type != MAT_AIR);
	//	//	}
	//	//}
	//}

	//printf("\n");
}

void simulation_delete(simulation_t* sim) {
	//arena_free(sim->arena);
}