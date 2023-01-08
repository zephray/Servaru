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

// mipmap storage
// xx R0 R1 R1
// G0 B0 R1 R1
// G1 G1 B1 B1
// G1 G1 B1 B1

static VEC3 s3d_tex_lookup_single(uint32_t tmu_id, int level, int x, int y) {
    VEC3 result = {0.0f, 0.0f, 0.0f};
    TMU *tmu = &s3d_context.tmu[tmu_id];
    size_t offset = 1u << level;
    if (x < 0) x = 0;
    if (x >= tmu->width) x = tmu->width - 1;
    if (y < 0) x = 0;
    if (y >= tmu->height) x = tmu->height - 1;
    uint8_t r = s3d_context.vram[tmu->address + (y * tmu->width * 2 + offset + x)];
    uint8_t g = s3d_context.vram[tmu->address + ((offset + y) * tmu->width * 2 + x)];
    uint8_t b = s3d_context.vram[tmu->address + ((offset + y) * tmu->width * 2 + offset + x)];
    result.x = (float)r / 255.0f;
    result.y = (float)g / 255.0f;
    result.z = (float)b / 255.0f;
    return result;
}

extern int min_level, max_level;

// The following are parts of the GPU
VEC4 s3d_tex_lookup(uint32_t tmu_id, float dmax, VEC2 tex_coord) {
    TMU *tmu = &s3d_context.tmu[tmu_id];
    VEC4 result = {0.0f, 0.0f, 0.0f, 0.0f};
    if (!tmu->enabled)
        return result;
#if 1
    // Determine mipmap levels
    dmax = dmax * tmu->width;
    dmax = logf(dmax) / logf(2.0);
    dmax = ceil(dmax);
    int level = (int)dmax;
    if (level < 0) level = 0;

    if (level > tmu->mipmap_levels)
        level = tmu->mipmap_levels;
    uint32_t level_factor = tmu->mipmap_levels - level;

    if (min_level > (int)level_factor)
        min_level = level_factor;
    if (max_level < (int)level_factor)
        max_level = level_factor;

    // TODO: Implement proper/ configurable clamping?
    tex_coord.x = fmodf(fabsf(tex_coord.x), 1.0f);
    tex_coord.y = fmodf(fabsf(tex_coord.y), 1.0f);
    float texel_x = tex_coord.x * (tmu->width >> level);
    float texel_y = tex_coord.y * (tmu->height >> level);
    uint32_t x = (uint32_t)texel_x; // Truncate
    uint32_t y = (uint32_t)texel_y;
    texel_x = texel_x - (float)x;
    texel_y = texel_y - (float)y;

    VEC3 ul = s3d_tex_lookup_single(tmu_id, level_factor, x, y);
    VEC3 ur = s3d_tex_lookup_single(tmu_id, level_factor, x + 1, y);
    VEC3 ll = s3d_tex_lookup_single(tmu_id, level_factor, x, y + 1);
    VEC3 lr = s3d_tex_lookup_single(tmu_id, level_factor, x + 1, y + 1);

    VEC3 u = vec3_lerp(texel_x, ur, ul);
    VEC3 l = vec3_lerp(texel_x, lr, ll);
    VEC3 p = vec3_lerp(texel_y, l, u);

    //VEC3 p = s3d_tex_lookup_single(tmu_id, level_factor, x, y);

    result.x = p.x;
    result.y = p.y;
    result.z = p.z;

    //printf("TEX %d, %d, %.2f, %.2f -> %.2f, %.2f, %.2f\n", tmu_id, level,
    //        tex_coord.x, tex_coord.y, result.x, result.y, result.z);

    // TODO: Implement alpha channel
    // TODO: Implement reading from float buffer
#else
    result.x = tex_coord.x;
    result.y = tex_coord.y;
    result.z = 0;
#endif
    
    return result;
}
