#pragma once

#include "../../src/materials_common.h"


material_t init_AIR(void){
    material_t mat;
	ZERO_OUT_VAR(mat);

    mat.name = "AIR";
    mat.color = (rgba_t){0, 0, 0, 0};
	mat.death_chance = 0;
	mat.density = 0;
	mat.viscosity = 0;
	mat.advesity = 0;
	mat.flags = NONE;
	mat.flamable = false;
	mat.meltable = false;
	mat.flaming = false;

    return mat;
}


void update_AIR(MATERIAL_UPDATE_FUNC_ARGS){
	
}