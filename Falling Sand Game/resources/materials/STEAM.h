#pragma once

#include "../../src/materials_common.h"

static void tick_STEAM(MATERIAL_TICK_FUNC_ARGS) {

}

inline material_t init_STEAM(void){
    material_t mat;
	ZERO_OUT_VAR(mat);

    mat.name = "STEAM";
    mat.color = (rgba_t){229, 230, 235, 255};
	mat.death_chance = 0;
	mat.mass = DENSITY_MAX * 0.05f;
	mat.viscosity = 0.35f;
	mat.advesity = 0.8f;
	mat.flags = GAS;
	mat.flamable = false;
	mat.meltable = false;
	mat.flaming = false;

	mat.aim_temp = ZERO_TEMP + 100.f;
	mat.conversions.temperature.low = ZERO_TEMP + 100.f - 2.f;
	mat.conversions.temperature.low_conversion = MAT_WATER;

	mat.tick = tick_STEAM;

    return mat;
}