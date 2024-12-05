#pragma once

#include "../../src/materials_common.h"

static void tick_FIRE(MATERIAL_TICK_FUNC_ARGS) {
	
}

inline material_t init_FIRE(void){
    material_t mat;
	ZERO_OUT_VAR(mat);

    mat.name = "FIRE";
    mat.color = (rgba_t){255, 95, 8, 255};
	mat.death_chance = 20;
	mat.mass = -DENSITY_MAX * 0.1f;
	mat.viscosity = 0;
	mat.advection = 0.1f;
	mat.flags = MAT_FLAG_GAS;
	mat.flamable = false;
	mat.meltable = false;
	mat.flaming = true;
	mat.tick = tick_FIRE;

    return mat;
}