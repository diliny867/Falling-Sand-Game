#version 330

#include "glsl_common.h"

layout(location = 0) in vec3    vert_pos;
layout(location = 1) in vec2    vert_tex_coord;
layout(location = 2) in int     vert_grid_data;
layout(location = 3) in float   temperature_grid_data;
layout(location = 4) in int     flag_grid_data;
//layout(location = 5) in vec2    xy_offsets_grid_data;


uniform mat4 mvp;

uniform ivec2 grid_size = ivec2(1000, 1000);

#define MATERIALS_MAX 100
uniform vec4 material_colors[MATERIALS_MAX];

out vec2 frag_tex_coord;
out vec4 frag_color;

vec4 fire_color = vec4(1.0, 0.3725, 0.0314, 0.5);

void main(){
    frag_tex_coord = vert_tex_coord;
    //frag_color = material_colors[data[gl_InstanceID]];
    frag_color = material_colors[vert_grid_data];
    frag_color = mix(frag_color, fire_color, int((flag_grid_data & MAT_FLAG_BURNING) != 0));

    int grid_y = gl_InstanceID / grid_size.x;
    int grid_x = gl_InstanceID - grid_y * grid_size.x;
    gl_Position = mvp * vec4(vec2(vert_pos) + vec2(grid_x, grid_y), 0.0, 1.0);
}
