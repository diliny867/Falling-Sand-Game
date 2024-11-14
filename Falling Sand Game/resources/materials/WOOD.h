#pragma once

#include "../../src/materials_common.h"


material_t init_WOOD(void){
    material_t mat;
	ZERO_OUT_VAR(mat);

    mat.name = "WOOD";
    mat.color = (rgba_t){187, 134, 71, 255};
	mat.death_chance = 0;
	mat.density = DENSITY_MAX * 0.5;
	mat.viscosity = 0;
	mat.advesity = 0;
	mat.flags = STATIC | SOLID;
	mat.flamable = true;
	mat.meltable = false;
	mat.flaming = false;

    return mat;
}