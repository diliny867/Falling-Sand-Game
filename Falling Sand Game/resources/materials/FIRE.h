#pragma once

#include "../../src/materials_common.h"


material_t init_FIRE(void){
    material_t mat;
	ZERO_OUT_VAR(mat);

    mat.name = "FIRE";
    mat.color = (rgba_t){255, 95, 8, 255};
	mat.death_chance = 20;
	mat.density = -DENSITY_MAX * 0.1f;
	mat.viscosity = 0;
	mat.advesity = 0.1f;
	mat.flags = GAS;
	mat.flamable = false;
	mat.meltable = false;
	mat.flaming = true;

    return mat;
}