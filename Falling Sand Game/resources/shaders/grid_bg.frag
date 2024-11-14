#version 330

in vec2 frag_tex_coord;
in vec4 frag_color;

out vec4 out_color;

void main(){
    vec2 frag_pos = frag_tex_coord;

    vec4 color = frag_color;
    //vec3 color = vec3(0.3);

    if(color.a <= 0.001){
        discard;
    }

    out_color = color;
}