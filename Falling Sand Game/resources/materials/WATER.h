#pragma once

#include "../../src/materials_common.h"

static void tick_WATER(MATERIAL_TICK_FUNC_ARGS) {

}

inline material_t init_WATER(void){
    material_t mat;
	ZERO_OUT_VAR(mat);

    mat.name = "WATER";
    mat.color = (rgba_t){98, 179, 222, 255};
	mat.death_chance = 0;
	mat.mass = DENSITY_MAX * 0.3f;
	mat.viscosity = 0.35f;
	mat.advection = 0.8f;
	mat.flags = MAT_FLAG_LIQUID;
	mat.flamable = false;
	mat.meltable = false;
	mat.flaming = false;

	mat.aim_temp = ROOM_TEMP;
	mat.conversions.temperature.low = ZERO_TEMP;
	mat.conversions.temperature.low_conversion = MAT_ICE;
	mat.conversions.temperature.high = ZERO_TEMP + 100.f;
	mat.conversions.temperature.high = MAT_STEAM;

	mat.tick = tick_WATER;

    return mat;
}