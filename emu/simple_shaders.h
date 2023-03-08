#pragma once

typedef struct {
    MAT4 projection_view_matrix;
} UNIFORM;

void simple_vs(UNIFORM *uniforms, float *attributes, float *varying, VEC4 *position);
void simple_fs(UNIFORM *uniforms, float *varying, float *ddx, float *ddy, VEC3 *frag_color, float *frag_depth);
