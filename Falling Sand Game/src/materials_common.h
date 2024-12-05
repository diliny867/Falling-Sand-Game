#pragma once

#include <string.h>
#include "common.h"


#define SPEED_MAX UINT8_MAX
#define DEATH_CHANCE_MAX UINT8_MAX
#define DENSITY_MAX UINT8_MAX

typedef enum {
	MAT_FLAG_NONE =		0,
	MAT_FLAG_SOLID =	1 << 0,
	MAT_FLAG_LIQUID =	1 << 1,
	MAT_FLAG_GAS =		1 << 2,
	MAT_FLAG_STATIC =	1 << 3,
} material_flag_e_t;


typedef struct {
	uint8_t r, g, b, a;

} rgba_t;

#define ZERO_OUT_VAR(var) memset(&var, 0, sizeof(var))

typedef struct simulation_t simulation_t;
#define MATERIAL_TICK_FUNC_ARGS simulation_t* sim, int index, int x, int y
typedef void (*material_tick_func_t)(MATERIAL_TICK_FUNC_ARGS);

#define ZERO_TEMP 273.15f
#define ROOM_TEMP_CELSIUS 20.f
#define ROOM_TEMP (ZERO_TEMP + ROOM_TEMP_CELSIUS)

typedef enum material_type_e_t material_type_e_t;
typedef struct {
	float low;
	material_type_e_t low_conversion;
	float high;
	material_type_e_t high_conversion;
} mat_conversion_t;

typedef struct {
	char* name;
	rgba_t color;
	float mass;
	float viscosity;
	float advection; //how it spreads to on x axis (0 - does not spread, 1 - spreads the same as on y)
	float aim_temp;
	float bounciness;
	float roundness;
	float friction;
	uint8_t death_chance; // survivability chance: 0 to 255 (255 is 100%)
	uint32_t flags;
	bool flamable;
	bool flaming;
	bool meltable;
	bool melting;
	struct {
		mat_conversion_t temperature;
		mat_conversion_t pressure;
	} conversions;
	material_tick_func_t tick;
} material_t;


// expands each to MAT_name,
#define ENUM_FOREACH_MATERIAL(OP) \
        OP(AIR)   \
        OP(CONCRETE)  \
        OP(WATER)   \
        OP(ICE)   \
        OP(STEAM)   \
        OP(SAND)  \
        OP(ROCK)  \
        OP(WOOD)  \
        OP(FIRE)


#define ENUM_GEN(val) MAT_##val,
#define ENUM_GEN_STRING(val) #val,
#define ENUM_GEN_INIT_FUNCS(val) extern material_t init_##val(void);
//#define ENUM_GEN_UPDATE_FUNCS(val) extern void update_##val(MATERIAL_UPDATE_FUNC_ARGS);
#define ENUM_INIT_GET_MATERIALS(val) materials[MAT_##val] = init_##val();

typedef enum material_type_e_t{
	ENUM_FOREACH_MATERIAL(ENUM_GEN)
	MATERIALS_COUNT // add last enum value to count materials
} material_type_e_t;
static const char* MATERIAL_NAMES_STRING[] ={
	ENUM_FOREACH_MATERIAL(ENUM_GEN_STRING)
};

ENUM_FOREACH_MATERIAL(ENUM_GEN_INIT_FUNCS)
//ENUM_FOREACH_MATERIAL(ENUM_GEN_UPDATE_FUNCS)

void init_get_materials(simulation_t* sim);
