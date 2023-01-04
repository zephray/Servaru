#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "engine.h"
#include "simple_shaders.h"

//#define DEBUG

// TODO: move gl_Position out of varying
void simple_vs(UNIFORM *uniforms, float *attributes, float *varying, VEC4 *position) {
    // Input layout:
    VEC3 *a_position = (VEC3 *)&attributes[0];
    VEC2 *a_tex_coords = (VEC2 *)&attributes[3];
    // Output layout
    VEC2 *tex_coords = (VEC2 *)&varying[0];

    VEC4 pos = {a_position->x, a_position->y, a_position->z, 1.0};
    *position = mat4_multiply_by_vec4(uniforms->projection_view_matrix, pos);
    *tex_coords = *a_tex_coords;

#ifdef DEBUG
    printf("Vertex shader run\n");
    print_mat4(&uniforms->projection_view_matrix, "U: projection_view");
    print_vec3(*a_position, "I: a_position");
    print_vec2(*a_tex_coords, "I: a_tex_coords");
    print_vec4(*position, "O: position");
#endif
}

void simple_fs(UNIFORM *uniforms, float *varying, float *ddx, float *ddy, VEC3 *frag_color, float *frag_depth) {
    // Input layout:
    VEC2 *tex_coords = (VEC2 *)&varying[0];
    //print_vec2(*tex_coords, "Tex Coords");
    
    VEC4 tex_result = s3d_tex_lookup(0, *tex_coords);
    frag_color->x = tex_result.x;
    frag_color->y = tex_result.y;
    frag_color->z = tex_result.z;
}
