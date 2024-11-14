#pragma once

#include "../../src/materials_common.h"


material_t init_WATER(void){
    material_t mat;
	ZERO_OUT_VAR(mat);

    mat.name = "WATER";
    mat.color = (rgba_t){98, 179, 222, 255};
	mat.death_chance = 0;
	mat.density = DENSITY_MAX * 0.3f;
	mat.viscosity = 0.35f;
	mat.advesity = 0.8f;
	mat.flags = LIQUID;
	mat.flamable = false;
	mat.meltable = false;
	mat.flaming = false;

    return mat;
}