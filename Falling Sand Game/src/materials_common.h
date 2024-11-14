#pragma once

#include <string.h>

#include "../../src/simulation.h"


#define SPEED_MAX UINT8_MAX
#define DEATH_CHANCE_MAX UINT8_MAX
#define DENSITY_MAX UINT8_MAX

typedef enum {
	NONE =      0,
    SOLID =     1 << 0,
    LIQUID =    1 << 1,
    GAS =     	1 << 2,
	STATIC = 	1 << 3,

} material_flag_e_t;


typedef struct {
	uint8_t r, g, b, a;

} rgba_t;

#define ZERO_OUT_VAR(var) memset(&var, 0, sizeof(var))

#define MATERIAL_UPDATE_FUNC_ARGS simulation_t* sim, int x, int y, int index
typedef void* (*material_update_func_t)(MATERIAL_UPDATE_FUNC_ARGS);


typedef struct {
    char* name;
	rgba_t color;
	float density; // every particle has the same volume, so dont care about it
	float viscosity;
    float advesity; //how it spreads to on x axis (0 - does not spread, 1 - spreads the same as on y)
	uint8_t death_chance; // survivability chance: 0 to 255 (255 is 100%)
	uint32_t flags;
	bool flamable;
	bool meltable;
	bool flaming;

} material_t;


