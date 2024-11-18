#include "materials_common.h"

#include "simulation.h"

void init_get_materials(simulation_t* sim){ 
    material_t* materials = sim->materials;
    ENUM_FOREACH_MATERIAL(ENUM_INIT_GET_MATERIALS)
}
