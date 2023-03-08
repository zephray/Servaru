//
// Servaru
// Copyright 2022 Wenting Zhang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "vecmath.h"
#include "s3d.h"
#include "utils.h"
#include "s3d_private.h"

// TODO: Add bias
static float vec4_dot_w_bias(VEC4 *r1, VEC4 *r2, float w_bias) {
    return r1->x * r2->x +
            r1->y * r2->y +
            r1->z * r2->z +
            (r1->w + w_bias) * r2->w;
}

static bool is_vertex_inside_edge(VEC4 *edge, VEC4 *vertex, float w_bias) {
    return (vec4_dot_w_bias(vertex, edge, w_bias) > 0.0f);
}

static POST_VS_VERTEX get_intersection(VEC4 *edge, POST_VS_VERTEX *v0,
        POST_VS_VERTEX *v1, float w_bias) {
    float dp = vec4_dot_w_bias(&v0->position, edge, w_bias);
    float dp_prev = vec4_dot_w_bias(&v1->position, edge, w_bias);
    float factor = dp_prev / (dp_prev - dp);
    POST_VS_VERTEX result;
    result.position = vec4_lerp(factor, v0->position, v1->position);
    for (int i = 0; i < s3d_context.varying_count; i++)
        result.varying[i] = float_lerp(factor, v0->varying[i], v1->varying[i]);
    return result;
}

void s3d_setup_triangle(POST_VS_VERTEX *v0, POST_VS_VERTEX *v1,
        POST_VS_VERTEX *v2) {
    // For current resolution
    FBO fbo = ((FBO *)s3d_context.fbo.buf)[s3d_context.active_fbo];

    POST_VS_VERTEX position_a[9] = {0};
    POST_VS_VERTEX position_b[9] = {0};
    position_a[0] = *v0;
    position_a[1] = *v1;
    position_a[2] = *v2;

    // Clip using Sutherland-Hodgman algorithm
    // Otherwise, triangles that's partially visible won't be rendered correctly
    // Each clipping plane introduces up to 1 new vertex to the polygon.
    // Starting with a triangle, and may ended up getting 9 vertices
    uint32_t polygon_count;
    VEC4 clipping_edges[7] = {
        {-1.f,  0.f,  0.f,  1.f}, // 0 x = +w
        { 1.f,  0.f,  0.f,  1.f}, // 1 x = -w
        { 0.f, -1.f,  0.f,  1.f}, // 2 y = +w
        { 0.f,  1.f,  0.f,  1.f}, // 3 y = -w
        { 0.f,  0.f,  1.f,  0.f}, // 4 z = 0
        { 0.f,  0.f,  1.f,  1.f}, // 5 z = -w
        { 0.f,  0.f,  0.f,  1.f}, // 6 w = epsilon // this needs bias
    };

    uint32_t bias_u = 0x1;
    float min_bias = *(float *)(&bias_u);

    POST_VS_VERTEX *p_input_position = position_b;
    int input_count = 0;
    POST_VS_VERTEX *p_output_position = position_a;
    int output_count = 3;
    for (int i = 0; i < 7; i++) {
        float bias = (i == 6) ? 0.1f : 0.0f;

        // Clip
        // Output from last iteration become input of this iteration
        POST_VS_VERTEX *p_vec4_tmp = p_input_position;
        p_input_position = p_output_position;
        p_output_position = p_vec4_tmp;
        int int_tmp = input_count;
        input_count = output_count;
        output_count = int_tmp;
        // Clear output
        memset(p_output_position, 0, output_count * sizeof(POST_VS_VERTEX));
        output_count = 0;

#ifdef DEBUG
        printf("Round %d, %d inputs:\n", i, input_count);
        for (int i = 0; i < input_count; i++) {
            print_vec4(p_input_position[i].position, "input");
        }
#endif
        
        // TODO: Optimize this. This would ended up being in the GPU
        POST_VS_VERTEX* reference_vertex = &p_input_position[input_count - 1];
        for (int j = 0; j < input_count; j++) {
            if (is_vertex_inside_edge(&clipping_edges[i], &p_input_position[j].position, bias)) {
                if (!is_vertex_inside_edge(&clipping_edges[i], &reference_vertex->position, bias)) {
                    p_output_position[output_count++] =
                            get_intersection(&clipping_edges[i],
                            &p_input_position[j], reference_vertex, bias);
                }
                p_output_position[output_count++] = p_input_position[j];
            }
            else if (is_vertex_inside_edge(&clipping_edges[i], &reference_vertex->position, bias)) {
                p_output_position[output_count++] =
                        get_intersection(&clipping_edges[i],
                        &p_input_position[j], reference_vertex, bias);
            }
            reference_vertex = &p_input_position[j];
        }
    }

#ifdef DEBUG
    printf("Generated %d vertices\n", output_count);
    for (int i = 0; i < output_count; i++) {
        print_vec4(p_output_position[i].position, "output");
    }
#endif

    for (int j = 0; j < output_count; j++) {
        float inv_w = 1.0f / p_output_position[j].position.w;
        p_output_position[j].position.x = (p_output_position[j].position.x * inv_w + 1.0f) * fbo.width / 2;
        p_output_position[j].position.y = (1.0f - p_output_position[j].position.y * inv_w) * fbo.height / 2;
        p_output_position[j].position.z = (p_output_position[j].position.z * inv_w);
        p_output_position[j].position.w = inv_w;
        for (int k = 0; k < s3d_context.varying_count; k++) {
            p_output_position[j].varying[k] *= inv_w;
        }
    }
    for (int j = 0; j < output_count; j++) {
        p_output_position[j].screen_position[0] = (int32_t)(p_output_position[j].position.x + 0.5);
        p_output_position[j].screen_position[1] = (int32_t)(p_output_position[j].position.y + 0.5);
    }
    // END OF VS STAGE

    // FIFO should be able to hold at least 7 triangles at a time to avoid
    // blocking
    // Should this be a hardware FIFO (block RAM based)?
    // Say supporting maximum of 8 varyings... each 16 bytes, that's 128B
    // *7 = almost a KB. Sounds probably reasonable if allocate a 2KB FIFO?

    // ISSUE: I have a polygon here... how to select the right edges to produce an triangle?

    for (int i = 0; i < output_count - 2; i++) {
        //if (i == 0) continue;

        // Process output triangle
#ifdef DEBUG
        printf("Triangle %d\n", i);
        print_vec4(p_output_position[0].position, "Vertex 0");
        print_vec4(p_output_position[i + 1].position, "Vertex 1");
        print_vec4(p_output_position[i + 2].position, "Vertex 2");
#endif

#if 0
        // Culling
        VEC3 vtx1 = {position_a[0].position.x / position_a[0].position.w, position_a[0].position.y / position_a[0].position.w, 0};
        VEC3 vtx2 = {position_a[1].position.x / position_a[1].position.w, position_a[1].position.y / position_a[1].position.w, 0};
        VEC3 vtx3 = {position_a[2].position.x / position_a[2].position.w, position_a[2].position.y / position_a[2].position.w, 0};
        VEC3 vec1 = vec3_sub(vtx2, vtx1);
        VEC3 vec2 = vec3_sub(vtx3, vtx1);
        float signedarea = vec1.x * vec2.y - vec1.y * vec2.x;
        if (signedarea <= 0.f) {
            printf("Culled\n");
            // Cull away
            //continue;
        }
#endif

        // Rasterization:
        s3d_rasterize_triangle(
                &p_output_position[0],
                &p_output_position[i + 2],
                &p_output_position[i + 1]);
#if 0
        int32_t pos0x = p_output_position[0].screen_position[0];
        int32_t pos0y = p_output_position[0].screen_position[1];
        int32_t pos1x = p_output_position[i + 2].screen_position[0];
        int32_t pos1y = p_output_position[i + 2].screen_position[1];
        int32_t pos2x = p_output_position[i + 1].screen_position[0];
        int32_t pos2y = p_output_position[i + 1].screen_position[1];
        s3d_line(&fbo, pos0x, pos0y, pos1x, pos1y, 0x0000ffff);
        s3d_line(&fbo, pos1x, pos1y, pos2x, pos2y, 0x00ff00ff);
        s3d_line(&fbo, pos2x, pos2y, pos0x, pos0y, 0xff0000ff);
#endif
    }
}