#pragma once

#include "../../src/materials_common.h"


material_t init_CONCRETE(void){
    material_t mat;
	ZERO_OUT_VAR(mat);

    mat.name = "CONCRETE";
    mat.color = (rgba_t){166, 166, 163, 255};
	mat.death_chance = 0;
	mat.density = DENSITY_MAX;
	mat.viscosity = 0;
	mat.advesity = 0;
	mat.flags = STATIC | SOLID;
	mat.flamable = false;
	mat.meltable = false;
	mat.flaming = false;

    return mat;
}