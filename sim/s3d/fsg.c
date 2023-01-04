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
#include <assert.h>
#include <math.h>
#include "vecmath.h"
#include "s3d.h"
#include "utils.h"
#include "s3d_private.h"
#include "simple_shaders.h"

static bool z_test(float *z_addr, float depth) {
    float old_z = *z_addr;
    // Always use LESS for now
    // No Z masking for now
    if (depth < old_z) {
        *z_addr = depth;
        //printf("Fragment accepted: %.5f -> %.5f\n", old_z, depth);
        return true;
    }
    //printf("Fragment rejected: %.5f -> %.5f\n", old_z, depth);
    return false;
}

// Accept a group of pixels (2x2) and starts processing
void s3d_process_fragments(bool* masks, int32_t x, int32_t y, int32_t *w0,
        int32_t *w1, int32_t *w2, POST_VS_VERTEX *v0, POST_VS_VERTEX *v1,
        POST_VS_VERTEX *v2) {
    // Reminder: triangle order
    // 0 1 EDGE
    // 2 3 FUNC
    int32_t xx[4] = {x, x + 1, x, x + 1};
    int32_t yy[4] = {y, y, y + 1, y + 1};

    FBO fbo = ((FBO *)s3d_context.fbo.buf)[s3d_context.active_fbo];
    float *z_buffer = (float *)&s3d_context.vram[fbo.depth_address];

#if 0
    for (int i = 0; i < 4; i++) {
        if (masks[i]) {
            s3d_set_pixel(&fbo, xx[i], yy[i], 0xffffffff); 
        }
    }
#else
    // Run setup process serially. On hardware they are processed in parallel.
    float interpolated_z_over_w[4];
    float frag_depth[4];
    bool early_z = false;
    for (int i = 0; i < 4; i++) {
        interpolated_z_over_w[i] = (v0->position.z * w0[i] +
                v1->position.z * w1[i] + v2->position.z * w2[i]) /
                (w0[i] + w1[i] + w2[i]);
        frag_depth[i] = interpolated_z_over_w[i];
        if (s3d_context.early_depth_test && masks[i]) {
            early_z |= z_test(&z_buffer[yy[i] * fbo.width + xx[i]], frag_depth[i]);
        }
    }

    // Reject pixels
    if (s3d_context.early_depth_test & !early_z) {
        // If all pixels are rejected by early Z, reject the group.
        return;
    }

    // Interpolate varyings
    // Interpolation should not be masked as they are still used for partial derivative
    float varying[4][s3d_context.varying_count];
    for (int i = 0; i < 4; i++) {
        VEC3 w_inverse = {v0->position.w, v1->position.w, v2->position.w};
        VEC3 baricentric_coord = {(float)w0[i], (float)w1[i], (float)w2[i]};
        float interpolated_w_inverse = 1.0f / vec3_dot(w_inverse, baricentric_coord);
    
        for (uint32_t j = 0; j < s3d_context.varying_count; j++) {
            //printf("Interpolating varying %d\n", i);
            VEC3 attr_over_w = {v0->varying[j], v1->varying[j], v2->varying[j]};
            //print_vec3(attr_over_w, "Attributions");
            float interpolated_attr_over_w = vec3_dot(attr_over_w, baricentric_coord);
            varying[i][j] = interpolated_attr_over_w * interpolated_w_inverse;
        }
    }

    // Calculate partial derivative
    float ddx[2][s3d_context.varying_count / 4];
    float ddy[2][s3d_context.varying_count / 4];
    // Question: how does this part parallelize?
    // Probably only calculate when needed (like, texture mapping.)
    for (uint32_t i = 0; i < s3d_context.varying_count / 4; i++) {
        ddx[0][i] = varying[1][i * 4] - varying[0][i * 4];
        ddx[1][i] = varying[3][i * 4] - varying[2][i * 4];
        ddy[0][i] = varying[2][i * 4 + 1] - varying[0][i * 4 + 1];
        ddy[1][i] = varying[3][i * 4 + 1] - varying[1][i * 4 + 1];
    }

    VEC3 frag_color[4];

    for (int i = 0; i < 4; i++) {
        // TODO: pass masks to simple_fs, allowing killing frag
        if (masks[i]) {
            simple_fs(
                s3d_context.uniforms,
                varying[i],
                ddx[i % 2],
                ddy[i / 2],
                &frag_color[i],
                &frag_depth[i]
            );
        }
    }

    // Late Z (if early Z is not enabled)
    if (!s3d_context.early_depth_test) {
        for (int i = 0; i < 4; i++) {
            if (masks[i]) {
                if (!z_test(&z_buffer[yy[i] * fbo.width + xx[i]], frag_depth[i]))
                    masks[i] = false;
            }
        }
    }

    // ROP
    for (int i = 0; i < 4; i++) {
        if (masks[i]) {
            // Color WB
            int32_t r = frag_color[i].x * 255.f;
            int32_t g = frag_color[i].y * 255.f;
            int32_t b = frag_color[i].z * 255.f;
            // Clamp down
            // TODO: Make this step optional?
            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            if (r < 0) r = 0;
            if (g < 0) g = 0;
            if (b < 0) b = 0;
            uint32_t color = (b << 24) | (g << 16) | (r << 8) | 0xff;
            s3d_set_pixel(&fbo, xx[i], yy[i], color); 
        }
    }
#endif
}