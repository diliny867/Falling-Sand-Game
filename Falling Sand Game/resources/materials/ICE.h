#pragma once

#include "../../src/materials_common.h"

static void tick_ICE(MATERIAL_TICK_FUNC_ARGS) {

}

inline material_t init_ICE(void){
    material_t mat;
	ZERO_OUT_VAR(mat);

    mat.name = "ICE";
    mat.color = (rgba_t){69, 209, 213, 255};
	mat.death_chance = 0;
	mat.mass = DENSITY_MAX * 0.5f;
	mat.viscosity = 0.35f;
	mat.advection = 0.8f;
	mat.flags = MAT_FLAG_STATIC | MAT_FLAG_SOLID;
	mat.flamable = false;
	mat.meltable = false;
	mat.flaming = false;
	mat.tick = tick_ICE;

	mat.aim_temp = ZERO_TEMP;
	mat.conversions.temperature.high = ZERO_TEMP + 2.f;
	mat.conversions.temperature.high = MAT_WATER;

    return mat;
}