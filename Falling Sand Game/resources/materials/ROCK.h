#pragma once

#include "../../src/materials_common.h"


material_t init_ROCK(void){
    material_t mat;
	ZERO_OUT_VAR(mat);

    mat.name = "ROCK";
    mat.color = (rgba_t){113, 103, 100, 255};
	mat.death_chance = 0;
	mat.density = DENSITY_MAX * 0.65;
	mat.viscosity = 0;
	mat.advesity = 0;
	mat.flags = SOLID;
	mat.flamable = false;
	mat.meltable = false;
	mat.flaming = false;

    return mat;
}