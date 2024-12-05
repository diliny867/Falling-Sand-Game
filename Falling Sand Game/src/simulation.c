#include "simulation.h"

#include <float.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <raymath.h>

#include "../include/bitset.h"

#define LIQUID_SIDE_FLOW_MULTIPLIER 5

//why i have this? because
#ifdef NDEBUG
# define NODEFAULT __assume(false)
#else
# define NODEFAULT assert(false)
#endif

static interaction_t WALL_INTERACTION = { COLL_CANT_MOVE, 0.2f, 0.f };

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
static void precompute_collisions(simulation_t* sim) {
	material_t* materials = sim->materials;
	material_t* mat_from, *mat_to;
	interaction_t* interaction;
	for(int mat_type_from = 0; mat_type_from < MATERIALS_COUNT + 1; mat_type_from++) {
		for(int mat_type_to = 0; mat_type_to < MATERIALS_COUNT + 1; mat_type_to++) {
			interaction = sim->interactions + INTERACTION_GET_I(mat_type_from, mat_type_to);
			if(mat_type_from == MATERIALS_COUNT) {
#ifdef BOUNDS_PASS_THROUGH
				mat_from = materials + MAT_AIR;
#else
				mat_from = materials + MAT_CONCRETE;
#endif
			}else {
				mat_from = materials + mat_type_from;
			}
			if(mat_type_to == MATERIALS_COUNT) {
#ifdef BOUNDS_PASS_THROUGH
				mat_to = materials + MAT_AIR;
#else
				mat_to = materials + MAT_CONCRETE;
#endif
			}else {
				mat_to = materials + mat_type_to;
			}
			interaction->collision = material_get_collision(mat_from, mat_to);
			//if(mat_type_from == MAT_SAND && mat_type_to == MAT_AIR) {
			//	printf("AGAGSGASga%d\n", interaction->collision);
			//}
			interaction->bounce_deviation = ((mat_from->roundness + mat_to->roundness) * 0.5f) * BOUNCE_DEVIATION_MAX;
			interaction->bounce = mat_from->bounciness * mat_to->bounciness; // * (1.f + interaction->bounce_deviation * rand_sign(&sim->rand_state));
			interaction->friction = mat_from->friction * mat_to->friction;
		}
	}
}
void simulation_init(simulation_t* sim) {
	sim->rand_state.a = time(NULL);

	sim->gravity = 0.05f;
	sim->air_drag = 0.95f;
	sim->terminal_v = 100000.f;

	memset(sim->grid, 0, GRID_SIZE * sizeof(material_type_e_t));
	memset(sim->particles, 0, GRID_SIZE * sizeof(pdata_t));

	for(int i = 0; i < GRID_CELLS_SIZE; i++) {
		sim->acceleration_map[i].x = 0.01f;
		sim->acceleration_map[i].y = 0.0f;
	}

	for(int i = 0; i < GRID_SIZE; i++) {
		sim->index_shuffle[i] = i;
	}
	array_shuffle(sim->index_shuffle, GRID_SIZE, &sim->rand_state);

	bitset_clear(sim->updated_cells_bitset, BITSET_SIZE_ARRAY(GRID_SIZE));

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

int to_check_x = -1;
int to_check_y = -1;
void simulation_print_cell_data(simulation_t* sim, int x, int y) {
	int index = GRID_GET_I(x, y);
	pdata_t* pdata = sim->particles + index;
	material_type_e_t mat_type = sim->grid[index];
	printf("x: %d y: %d i: %d\n", x, y, index);
	printf("type: %d: %s\n", mat_type, MATERIAL_NAMES_STRING[mat_type]);
	printf("x: %f y: %f vx: %f vy: %f\n", pdata->x, pdata->y, pdata->vx, pdata->vy);
	printf("life: %d flags: %u\n", pdata->life, pdata->flags);

	to_check_x = x;
	to_check_y = y;
}

static force_inline coll_res_typeonly_t can_move(simulation_t* sim, material_type_e_t mat_from, int x_to, int y_to) {
	if(outof_grid_xy(x_to, y_to)) {
		return COLL_CANT_MOVE;
	}
	material_type_e_t mat2 = sim->grid[GRID_GET_I(x_to, y_to)];
	coll_res_typeonly_t coll = sim->interactions[INTERACTION_GET_I(mat_from, mat2)].collision;
	return coll;
}
//static force_inline bool cell_occupied(simulation_t* sim, int x, int y) {
//	if(outof_grid_xy(x, y)) {
//		return true;
//	}
//	return sim->grid[GRID_GET_I(x, y)] != MAT_AIR;
//}
//#define FLT_COMPARE_ZERO(v) ((v) >= -FLT_EPSILON && (v) <= FLT_EPSILON)
//#define FLT_SIGN(f) ((*(uint32_t*)&(f))&0x80000000)
//#define OUTOF_POOL(pool, dir) (FLT_SIGN(pool) != FLT_SIGN(dir))
//#define POOL_EXTRACT(pool, dir) (OUTOF_POOL(pool, dir) * (pool))
//#define NEG_ONE_TO_POS_ONE(v) ((v) >= -1) && ((v) <= 1)
#define LAST_ITER(i, max) ((i) == ((max)-1))
#define ALWAYS_SCALE_MOVE
static force_inline coll_res_full_t clamp_move(simulation_t* sim, material_type_e_t mat_type, float start_x, float start_y, float end_x, float end_y, float* out_x, float* out_y) { //TODO: implement friction
	if((int)start_x == (int)end_x && (int)start_y == (int)end_y) {
		*out_x = end_x;
		*out_y = end_y;
		return COLL_CANT_MOVE;
	}

	float delta_x = end_x - start_x;
	float delta_y = end_y - start_y;

	// fixes adding +/-1 even when left to add is less than +/-1
	float pool_x = delta_x;
	float pool_y = delta_y;

	float dir_y, dir_x;
	float abs_dx = fabsf(delta_x);
	float abs_dy = fabsf(delta_y);
	float abs_max = fmaxf(abs_dy, abs_dx);
	int max_iters = (int)ceilf(abs_max);
	float inv_dmax = 1.f / abs_max;
#ifdef ALWAYS_SCALE_MOVE
	dir_x = delta_x * inv_dmax;
	dir_y = delta_y * inv_dmax;
#else
	if(abs_dy > 1.f || abs_dx > 1.f) {
		dir_x = delta_x * inv_dmax;
		dir_y = delta_y * inv_dmax;
	}else {
		dir_x = delta_x;
		dir_y = delta_y;
	}
#endif
	assert_text(fabsf(dir_x) <= 1.f && fabsf(dir_y) <= 1.f, "Direction increment cant exceed 1");

	if(abs_dy < 1.f && abs_dx < 1.f) {
		pool_x *= inv_dmax;
		pool_y *= inv_dmax;
	}

	coll_res_full_t res = COLL_CANT_MOVE;
	coll_res_typeonly_t res_x, res_y;
	int i = 0;
	float inc_x, inc_y;
	do {
		//if(NEG_ONE_TO_POS_ONE(pool_y)) {
		if(LAST_ITER(i, max_iters)) {
			inc_y = pool_y;
		}else {
			inc_y = dir_y;
		}
		start_y += inc_y;
		pool_y -= dir_y;

		res_y = can_move(sim, mat_type, (int)start_x, (int)start_y);
		if(res_y == COLL_CANT_MOVE) {
			if((int)start_y != (int)(start_y - inc_y)){ // half-assed self collision fix
				if(inc_y >= 0.f) {
					res |= COLL_FLAG_BOUNCE_DOWN;
				} else {
					res |= COLL_FLAG_BOUNCE_UP;
				}
			}
			start_y -= inc_y;

			dir_y = -dir_y;
			pool_y = -pool_y;
		} else {
			*out_y = start_y;
		}

		//if(NEG_ONE_TO_POS_ONE(pool_x)) {
		if(LAST_ITER(i, max_iters)) {
			inc_x = pool_x;
		}else {
			inc_x = dir_x;
		}
		start_x += inc_x;
		pool_x -= dir_x;

		res_x = can_move(sim, mat_type, (int)start_x, (int)start_y);
		if(res_x == COLL_CANT_MOVE) {
			if((int)start_x != (int)(start_x - inc_x)){
				if(inc_x >= 0.f) {
					res |= COLL_FLAG_BOUNCE_RIGHT;
				} else {
					res |= COLL_FLAG_BOUNCE_LEFT;
				}
			}
			start_x -= inc_x;

			dir_x = -dir_x;
			pool_x = -pool_x;
		} else {
			*out_x = start_x;
		}

		if((res_x | res_y) == COLL_CANT_MOVE) {
			if(!(res & COLL_SWAP)) {
				res = COLL_CANT_MOVE | (res & COLL_FLAG_MASK);
			}
			break;
		}else {
			if(res_x == COLL_REPLACE || (res_x == COLL_CANT_MOVE && res_y == COLL_REPLACE)) { // prioritise res_x
				res = COLL_REPLACE | (res & COLL_FLAG_MASK);
				break;
			}else {
				res |= COLL_SWAP;
			}
		}
	} while(++i < max_iters);

	//do {
	//	if(NEG_ONE_TO_POS_ONE(pool_x) && NEG_ONE_TO_POS_ONE(pool_y)) {
	//		start_x += pool_x;
	//		start_y += pool_y;
	//	}else {
	//		start_x += dir_x;
	//		start_y += dir_y;
	//	}
	//	pool_x -= dir_x;
	//	pool_y -= dir_y;

	//	res = can_move(sim, mat_type, (int)start_x, (int)start_y);
	//	if(res == COLL_CANT_MOVE) {
	//		int prev_x = (int)(start_x - dir_x);
	//		int prev_y = (int)(start_y - dir_y);
	//		if(prev_x != (int)start_x) {
	//			if(prev_x < (int)start_x) {
	//				res |= COLL_FLAG_BOUNCE_RIGHT;
	//			}else {
	//				res |= COLL_FLAG_BOUNCE_LEFT;
	//			}
	//			//if(can_move(sim, mat_type, (int)x_from, prev_y) == COLL_SWAP) {
	//			//	*x_res = x_from;
	//			//}
	//		}else {
	//			*out_x = start_x;
	//		}
	//		if(prev_y != (int)start_y) {
	//			if(prev_y < (int)start_y) {
	//				res |= COLL_FLAG_BOUNCE_DOWN;
	//			}else {
	//				res |= COLL_FLAG_BOUNCE_UP;
	//			}
	//			//if(can_move(sim, mat_type, prev_x, (int)y_from) == COLL_SWAP) {
	//			//	*y_res = y_from;
	//			//}
	//		}else {
	//			*out_y = start_y;
	//		}
	//		break;
	//	}else {
	//		*out_x = start_x;
	//		*out_y = start_y;
	//		if(res == COLL_REPLACE) {
	//			break;
	//		}
	//	}
	//} while((int)start_y != (int)end_y && (int)start_x != (int)end_x);
	//} while(FLT_SIGN(pool_x) != FLT_SIGN(dir_x) || FLT_SIGN(pool_y) != FLT_SIGN(dir_y));
	return res;
}
static force_inline void simulation_swap(simulation_t* sim, int index1, int index2) {
	//printf("%f %f %f %f\n", sim->particles[index1].x, sim->particles[index1].y, sim->particles[index2].x, sim->particles[index2].y);
	SWAP(material_type_e_t, sim->grid[index1], sim->grid[index2]);
	SWAP(pdata_t, sim->particles[index1], sim->particles[index2]);
}
static int a = 0;
static force_inline void move_cell(simulation_t* sim, const int index/*, int x, int y*/) {
	const material_type_e_t* grid = sim->grid;
	const material_type_e_t mat_type = grid[index];
	const material_t* materials = sim->materials;
	const material_t* mat = materials + mat_type;
	pdata_t* pdata = sim->particles + index;

	const int x = (int)pdata->x;
	const int y = (int)pdata->y;

	if(mat->flags & MAT_FLAG_STATIC) {
		if(mat->tick) {
			mat->tick(sim, index, x, y);
		}
		return;
	}

	const float terminal_v = sim->terminal_v;
	const xy_t acceleration = sim->acceleration_map[DATA_MAP_GET_I(x, y)];
	const float accel_x = (acceleration.x) * mat->mass * (mat->advection + 1.f);
	const float accel_y = (acceleration.y + sim->gravity) * mat->mass * (mat->advection + 1.f);
	pdata->vx = clampf(pdata->vx + accel_x, -terminal_v, terminal_v) * sim->air_drag;
	pdata->vy = clampf(pdata->vy + accel_y, -terminal_v, terminal_v) * sim->air_drag;
	const float end_x = pdata->x + pdata->vx;
	const float end_y = pdata->y + pdata->vy;
	float move_res_x = pdata->x;
	float move_res_y = pdata->y;

	coll_res_full_t coll = clamp_move(sim, mat_type, pdata->x, pdata->y, end_x, end_y, &move_res_x, &move_res_y);
	pdata->x = move_res_x;
	pdata->y = move_res_y;
	const int new_ix = (int)pdata->x;
	const int new_iy = (int)pdata->y;
	if(new_ix == x && new_iy == y) {
		if(mat->tick) {
			mat->tick(sim, index, x, y);
		}
		return;
	}
	
	interaction_t* interaction;
	////if(new_x > x && can_move(sim, mat_type, new_x + 1, new_y) == COLL_CANT_MOVE) {
	//if(coll & COLL_FLAG_BOUNCE_RIGHT){
	//	switch(coll & COLL_TYPE_MASK) {
	//	case COLL_CANT_MOVE:
	//		pdata->x = (float)new_ix + 0.5f;
	//		pdata->vx = 0;
	//		break;
	//	case COLL_REPLACE:
	//		pdata->x = (float)new_ix + 0.5f;
	//		pdata->vx = 0;
	//		break;
	//	case COLL_SWAP:
	//		interaction = sim->interactions + INTERACTION_GET_I(mat_type, grid[GRID_GET_I(new_ix + 1, new_iy)]);
	//		pdata->vx = -pdata->vx * interaction->bounce;
	//		float dev = 1.f + interaction->bounce_deviation * rand_sign(&sim->rand_state); // TODO: i guess also scale other velocity by dev
	//		pdata->vy = pdata->vy * dev;
	//		break;
	//	default: NODEFAULT;
	//	}
	////}else if(new_x < x && can_move(sim, mat_type, new_x - 1, new_y) == COLL_CANT_MOVE) {
	//}else if(coll & COLL_FLAG_BOUNCE_LEFT){
	//	switch(coll & COLL_TYPE_MASK) {
	//	case COLL_CANT_MOVE:
	//		pdata->x = (float)new_ix + 0.5f;
	//		pdata->vx = 0;
	//		break;
	//	case COLL_REPLACE:
	//		pdata->x = (float)new_ix + 0.5f;
	//		pdata->vx = 0;
	//		break;
	//	case COLL_SWAP:
	//		interaction = sim->interactions + INTERACTION_GET_I(mat_type, grid[GRID_GET_I((new_ix - 1 >= 0) ? new_ix - 1 : GRID_WIDTH, new_iy)]);
	//		pdata->vx = -pdata->vx * interaction->bounce;
	//		float dev = 1.f + interaction->bounce_deviation * rand_sign(&sim->rand_state);
	//		pdata->vy = pdata->vy * dev;
	//		break;
	//	default: NODEFAULT;
	//	}
	//}
	////if(new_y > y && can_move(sim, mat_type, new_x, new_y + 1) == COLL_CANT_MOVE) {
	//if(coll & COLL_FLAG_BOUNCE_DOWN){
	//	switch(coll & COLL_TYPE_MASK) {
	//	case COLL_CANT_MOVE:
	//		pdata->y = (float)new_iy + 0.5f;
	//		pdata->vy = 0;
	//		break;
	//	case COLL_REPLACE:
	//		pdata->y = (float)new_iy + 0.5f;
	//		pdata->vy = 0;
	//		break;
	//	case COLL_SWAP:
	//		interaction = sim->interactions + INTERACTION_GET_I(mat_type, grid[GRID_GET_I(new_ix, new_iy + 1)]);
	//		pdata->vy = -pdata->vy * interaction->bounce;
	//		float dev = 1.f + interaction->bounce_deviation * rand_sign(&sim->rand_state);
	//		pdata->vx = pdata->vx * dev;
	//		break;
	//	default: NODEFAULT;
	//	}
	////}else if(new_y < y && can_move(sim, mat_type, new_y, new_y - 1) == COLL_CANT_MOVE) {
	//}else if(coll & COLL_FLAG_BOUNCE_UP){
	//	switch(coll & COLL_TYPE_MASK) {
	//	case COLL_CANT_MOVE:
	//		pdata->y = (float)new_iy + 0.5f;
	//		pdata->vy = 0;
	//		break;
	//	case COLL_REPLACE:
	//		pdata->y = (float)new_iy + 0.5f;
	//		pdata->vy = 0;
	//		break;
	//	case COLL_SWAP:
	//		interaction = sim->interactions + INTERACTION_GET_I(mat_type, grid[GRID_GET_I(new_ix, (new_iy - 1 >= 0) ? new_iy - 1 : GRID_HEIGHT)]);
	//		pdata->vy = -pdata->vy * interaction->bounce;
	//		float dev = 1.f + interaction->bounce_deviation * rand_sign(&sim->rand_state);
	//		pdata->vx = pdata->vx * dev;
	//		break;
	//	default: NODEFAULT;
	//	}
	//}

	switch(coll & COLL_TYPE_MASK) {
	case COLL_CANT_MOVE:

		if(coll & (COLL_FLAG_BOUNCE_RIGHT | COLL_FLAG_BOUNCE_LEFT)){
			pdata->x = (float)new_ix + 0.5f;
			pdata->vx = 0;
		}
		if(coll & (COLL_FLAG_BOUNCE_DOWN | COLL_FLAG_BOUNCE_UP)) {
			pdata->y = (float)new_iy + 0.5f;
			pdata->vy = 0;
		}
		break;
	case COLL_REPLACE: 
		if(coll & (COLL_FLAG_BOUNCE_RIGHT | COLL_FLAG_BOUNCE_LEFT)){
			pdata->x = (float)new_ix + 0.5f;
			pdata->vx = 0;
		}
		if(coll & (COLL_FLAG_BOUNCE_DOWN | COLL_FLAG_BOUNCE_UP)) {
			pdata->y = (float)new_iy + 0.5f;
			pdata->vy = 0;
		}
		break;
	case COLL_SWAP: 
		if(coll & COLL_FLAG_BOUNCE_RIGHT){
			interaction = sim->interactions + INTERACTION_GET_I(mat_type, grid[GRID_GET_I(new_ix + 1, new_iy)]);
			pdata->vx = -pdata->vx * interaction->bounce;
			printf("R%f\n", interaction->bounce);
			float dev = 1.f + interaction->bounce_deviation * rand_sign(&sim->rand_state); // TODO: i guess also scale other velocity by dev
			pdata->vy = pdata->vy * dev;
		} else if(coll & COLL_FLAG_BOUNCE_LEFT) {
			interaction = sim->interactions + INTERACTION_GET_I(mat_type, grid[GRID_GET_I((new_ix - 1 >= 0) ? new_ix - 1 : GRID_WIDTH, new_iy)]);
			pdata->vx = -pdata->vx * interaction->bounce;
			printf("L%f\n", interaction->bounce);
			float dev = 1.f + interaction->bounce_deviation * rand_sign(&sim->rand_state);
			pdata->vy = pdata->vy * dev;
		}
		if(coll & COLL_FLAG_BOUNCE_DOWN) {
			interaction = sim->interactions + INTERACTION_GET_I(mat_type, grid[GRID_GET_I(new_ix, new_iy + 1)]);
			pdata->vy = -pdata->vy * interaction->bounce;
			printf("D%f\n", interaction->bounce);
			float dev = 1.f + interaction->bounce_deviation * rand_sign(&sim->rand_state);
			pdata->vx = pdata->vx * dev;
		} else if(coll & COLL_FLAG_BOUNCE_UP) {
			interaction = sim->interactions + INTERACTION_GET_I(mat_type, grid[GRID_GET_I(new_ix, (new_iy - 1 >= 0) ? new_iy - 1 : GRID_HEIGHT)]);
			pdata->vy = -pdata->vy * interaction->bounce;
			printf("U%f\n", interaction->bounce);
			float dev = 1.f + interaction->bounce_deviation * rand_sign(&sim->rand_state);
			pdata->vx = pdata->vx * dev;
		}
		break;
	default: NODEFAULT;
	}

	const int new_index = GRID_GET_I(new_ix, new_iy);
	if(mat->tick) {
		mat->tick(sim, new_index, new_ix, new_iy);
	}

	sim->particles[new_index].x = x;
	sim->particles[new_index].y = y;
	simulation_swap(sim,index,new_index);
}

static force_inline void process_cell(simulation_t* sim, int index/*, int x, int y*/) {
	//printf("\npre: %d %d\n", x, y);
	//printf("x: %d, y: %d, nx: %d, ny: %d\n", x, y, new_x, new_y);
	move_cell(sim, index/*, x, y*/);
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
				//bitset_set_weak(sim->updated_cells_bitset, index, mat_type != MAT_AIR);
				continue;
			}

			//GRID_INDEX_GET_XY(index, x, y);
			process_cell(sim, index/*, x, y*/);
			bitset_set(sim->updated_cells_bitset, index, 1);
		}
	}
	//printf("\n");
}

void simulation_free(simulation_t* sim) {
	//arena_free(sim->arena);
}