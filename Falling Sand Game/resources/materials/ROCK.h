#pragma once

#include "../../src/materials_common.h"


inline material_t init_ROCK(void){
    material_t mat;
	ZERO_OUT_VAR(mat);

    mat.name = "ROCK";
    mat.color = (rgba_t){113, 103, 100, 255};
	mat.death_chance = 0;
	mat.mass = DENSITY_MAX * 0.65;
	mat.viscosity = 0;
	mat.advection = 0;
	mat.flags = MAT_FLAG_SOLID;
	mat.flamable = false;
	mat.meltable = false;
	mat.flaming = false;

    return mat;
}