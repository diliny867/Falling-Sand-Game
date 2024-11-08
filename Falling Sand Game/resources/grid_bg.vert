#version 430

in vec3 vert_pos;
in vec2 vert_tex_coord;

layout(std430, binding = 4) buffer SSBO {
    int data[];
};

#define MATERIALS_MAX 100

uniform mat4 mvp;

uniform int grid_width = 1024;

uniform vec4 material_colors[MATERIALS_MAX];

out vec2 frag_tex_coord;
out vec4 frag_color;


void main(){
    frag_tex_coord = vert_tex_coord;
    frag_color = material_colors[data[gl_InstanceID]];

    gl_Position = mvp * vec4(vec2(vert_pos) + vec2(gl_InstanceID % grid_width, gl_InstanceID / grid_width), 0.0, 1.0);
}
