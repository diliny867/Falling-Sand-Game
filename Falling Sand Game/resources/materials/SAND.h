#pragma once

#include "../../src/materials_common.h"


material_t init_SAND(void){
    material_t mat;
	ZERO_OUT_VAR(mat);

    mat.name = "SAND";
    mat.color = (rgba_t){250, 242, 197, 255};
	mat.death_chance = 0;
	mat.density = DENSITY_MAX * 0.5;
	mat.viscosity = 0;
	mat.advesity = 0.5f;
	mat.flags = SOLID;
	mat.flamable = false;
	mat.meltable = false;
	mat.flaming = false;

    return mat;
}