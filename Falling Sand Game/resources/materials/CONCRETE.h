#pragma once

#include "../../src/materials_common.h"


inline material_t init_CONCRETE(void){
    material_t mat;
	ZERO_OUT_VAR(mat);

    mat.name = "CONCRETE";
    mat.color = (rgba_t){166, 166, 163, 255};
	mat.death_chance = 0;
	mat.mass = DENSITY_MAX;
	mat.viscosity = 0;
	mat.advection = 0;
	mat.bounciness = 1.f;
	mat.roundness = 1.f;
	mat.flags = MAT_FLAG_STATIC | MAT_FLAG_SOLID;
	mat.flamable = false;
	mat.flaming = false;
	mat.melting = false;
	mat.meltable = false;

    return mat;
}