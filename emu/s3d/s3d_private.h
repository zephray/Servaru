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

#define MIN(a, b) (a < b) ? (a) : (b)
#define MAX(a, b) (a > b) ? (a) : (b)

#define MAX_TEXTURE_SIZE (512)

#define VRAM_SIZE (256 * 1024 * 1024)
#define UNIFORM_SIZE (4 * 128)
#define MAX_VARYING (32) // Maximum num of floats, 32 means 8 vec4
#define TMU_COUNT (1)

#define READER_COUNTER (2)
// 2-reader in the system:
// 1 execution unit
// 1 interpolator
// Each SRAM is 1R1W. Read port belongs to its prospective read, write port
// is shared among all writers with a crossbar. Write may take several cycles.
// In the future this may be clustered.
#define SHARED_SIZE (MAX_VARYING * 4 * 2)

typedef struct {
    uint32_t vbo_id;
    uint32_t ebo_id;
    uint32_t attribute_size;
    uint32_t attribute_stride;
} VAO;

typedef struct {
    uint32_t address;
    uint32_t size;
} VBO;

typedef struct {
    uint32_t address;
    uint32_t size;
} EBO;

typedef struct {
    uint32_t color_address;
    uint32_t depth_address;
    uint32_t width;
    uint32_t height;
    uint32_t size;
} FBO;

typedef struct {
    uint32_t address;
    uint32_t width;
    uint32_t height;
    uint32_t mipmap_levels;
} TEX;

typedef struct {
    bool enabled;
    uint32_t address;
    uint16_t width;
    uint16_t height;
    uint8_t mipmap_levels;
} TMU;

typedef struct {
    /* Driver states */
    // Objects
    RESIZABLE_ARRAY vao;
    RESIZABLE_ARRAY vbo;
    RESIZABLE_ARRAY ebo;
    RESIZABLE_ARRAY fbo;
    RESIZABLE_ARRAY tex;
    uint32_t memptr;
    uint32_t active_fbo;
    uint32_t varying_count;

    /* Hardware states */
    uint8_t vram[VRAM_SIZE];
    uint8_t uniforms[UNIFORM_SIZE];
    uint8_t shared[READER_COUNTER][SHARED_SIZE];

    TMU tmu[TMU_COUNT];

    // Pipeline configs
    bool depth_test;
    bool early_depth_test;
    bool face_culling;
    bool perspective_correct;
} S3D_CONTEXT;

typedef struct {
    // TODO: Keep these as a union... if that ever matters
    VEC4 position; // Not kept after rasterization step, only for clipping
    int32_t screen_position[4];
    float varying[MAX_VARYING - 4];
} POST_VS_VERTEX;

extern S3D_CONTEXT s3d_context;

float float_lerp(float factor, float r1, float r2);
VEC3 vec3_lerp(float factor, VEC3 r1, VEC3 r2);
void swap(int *a, int *b);

uint32_t s3d_map_rgb(uint8_t r, uint8_t g, uint8_t b);
void s3d_set_pixel(FBO *fbo, int32_t x, int32_t y, uint32_t color);
void s3d_xline(FBO *fbo, int32_t x0, int32_t y0, int32_t x1, uint32_t color);
void s3d_yline(FBO *fbo, int32_t x0, int32_t y0, int32_t y1, uint32_t color);
void s3d_line(FBO *fbo, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);

void s3d_process_fragments(bool* masks, int32_t x, int32_t y, int32_t *w0,
        int32_t *w1, int32_t *w2, POST_VS_VERTEX *v0, POST_VS_VERTEX *v1,
        POST_VS_VERTEX *v2);
void s3d_rasterize_triangle(POST_VS_VERTEX *v0, POST_VS_VERTEX *v1,
        POST_VS_VERTEX *v2);
void s3d_setup_triangle(POST_VS_VERTEX *v0, POST_VS_VERTEX *v1,
        POST_VS_VERTEX *v2);
