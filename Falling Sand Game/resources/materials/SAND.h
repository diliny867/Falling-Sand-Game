#pragma once

#include "../../src/materials_common.h"


inline material_t init_SAND(void){
    material_t mat;
	ZERO_OUT_VAR(mat);

    mat.name = "SAND";
    mat.color = (rgba_t){250, 242, 197, 255};
	mat.death_chance = 0;
	mat.mass = DENSITY_MAX * 0.5f;
	mat.viscosity = 0;
	mat.advesity = 0.5f;
	mat.bounciness = 0.2f;
	mat.roundness = 0.5f;
	mat.flags = SOLID;
	mat.flamable = false;
	mat.meltable = false;
	mat.flaming = false;

    return mat;
}