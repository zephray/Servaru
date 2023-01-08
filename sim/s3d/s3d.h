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
#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    PF_RGB8,
    PF_RGBA8,
    PF_RGB16F,
    PF_RGBA16F,
    PF_RGB32F,
    PF_RGBA32F
} PIXEL_FORMAT;

// Initialize S3D, create window output
void s3d_init(uint32_t width, uint32_t height);
// Deinitialize S3D, close window
void s3d_deinit();
// Create framebuffer
uint32_t s3d_create_framebuffer(uint32_t width, uint32_t height, PIXEL_FORMAT format);
// Clear color buffer
void s3d_clear_color();
// Clear depth buffer
void s3d_clear_depth();
// Enable depth test
void s3d_depth_test(bool enable);
// Enable face culling
void s3d_face_culling(bool enable);
// Load indices buffer into VRAM
uint32_t s3d_load_ebo(void *buffer, size_t size);
// Load vertices buffer into VRAM
uint32_t s3d_load_vbo(void *buffer, size_t size);
// Bind ebo and vbo to vao
uint32_t s3d_bind_vao(uint32_t ebo_id, uint32_t vbo_id, uint32_t attr_size,
        uint32_t attr_stride);
// Load texture into VRAM
uint32_t s3d_load_tex(void *buffer, size_t width, size_t height,
        size_t channels, size_t byte_per_channel);
// Bind texture with TMU
void s3d_bind_texture(uint32_t tmu, uint32_t tex_id);
// Update uniform
void s3d_update_uniform(void *buffer, size_t size);
// Set active varying count
void s3d_set_varying_count(size_t count);
// Render to framebuffer
void s3d_render(uint32_t vao_id);
// Render copy
void s3d_render_copy(uint8_t *destination);
// Delete ebo from VRAM
void s3d_delete_ebo(uint32_t ebo_id);
// Delete vbo from VRAM
void s3d_delete_vbo(uint32_t vbo_id);
// Unbind vao
void s3d_delete_vao(uint32_t vao_id);
// Delete texture from VRAM
void s3d_delete_tex(uint32_t tex_id);

// For C shaders, to be removed later?
// Or keep... IDK
// Lookup could be up to 4x32 bit wide
VEC4 s3d_tex_lookup(uint32_t tmu_id, float dmax, VEC2 tex_coord);
