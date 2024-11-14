#pragma once

#include "../../include/arena.h"

#include "materials_common.h"

#include "../resources/ALL_MATERIALS.h"


// expands each to MAT_name,
#define ENUM_FOREACH_MATERIAL(OP) \
        OP(AIR)   \
        OP(CONCRETE)  \
        OP(WATER)   \
        OP(SAND)  \
        OP(ROCK)  \
        OP(WOOD)  \
        OP(FIRE)


#define ENUM_GEN(val) MAT_##val,
//#define ENUM_GEN_STRING(val) #val,
#define ENUM_GEN_INIT_FUNCS(val) extern material_t init_##val(void);
#define ENUM_GEN_UPDATE_FUNCS(val) extern void update_##val(MATERIAL_UPDATE_FUNC_ARGS);
#define ENUM_INIT_GET_MATERIALS(val) materials[MAT_##val] = init_##val();

typedef enum {
	ENUM_FOREACH_MATERIAL(ENUM_GEN)
    MATERIALS_COUNT // add last enum value to count materials
} material_type_e_t;
//static const char* MATERIAL_NAMES_STRING[] ={
//	ENUM_FOREACH_MATERIAL(ENUM_GEN_STRING)
//};

ENUM_FOREACH_MATERIAL(ENUM_GEN_INIT_FUNCS);
ENUM_FOREACH_MATERIAL(ENUM_GEN_UPDATE_FUNCS);

inline material_t* init_get_materials(arena_t* a){ 
    material_t* materials = (material_t*)arena_alloc(a, sizeof(material_t) * MATERIALS_COUNT);
    ENUM_FOREACH_MATERIAL(ENUM_INIT_GET_MATERIALS);
    return materials;
}
