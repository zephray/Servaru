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
#include <SDL.h>
#include "vecmath.h"
#include "s3d.h"
#include "utils.h"
#include "s3d_private.h"
#include "simple_shaders.h"

//#define DEBUG

S3D_CONTEXT s3d_context;

// Allocate from VRAM
// Simple linear allocator without being able to free...
static uint32_t s3d_malloc(uint32_t size) {
    uint32_t mem = s3d_context.memptr;
    assert((VRAM_SIZE - mem) > size);
    s3d_context.memptr += size;
    return mem;
}

static void s3d_free(uint32_t ptr) {
    return;
}

void s3d_init(uint32_t width, uint32_t height) {
    ra_init(&s3d_context.vao, sizeof(VAO));
    ra_init(&s3d_context.vbo, sizeof(VBO));
    ra_init(&s3d_context.ebo, sizeof(EBO));
    ra_init(&s3d_context.fbo, sizeof(FBO));
    ra_init(&s3d_context.tex, sizeof(TEX));
    s3d_context.depth_test = true;
    s3d_context.early_depth_test = true;
    s3d_context.face_culling = true;
    s3d_context.perspective_correct = true;
    s3d_context.active_fbo = s3d_create_framebuffer(width, height, PF_RGBA8);
    s3d_clear_color();
    s3d_clear_depth();
}

void s3d_deinit() {
    ra_deinit(&s3d_context.vao);
    ra_deinit(&s3d_context.vbo);
    ra_deinit(&s3d_context.ebo);
    ra_deinit(&s3d_context.fbo);
    ra_deinit(&s3d_context.tex);
}

static size_t get_pixel_width(PIXEL_FORMAT format) {
    switch (format)
    {
    case PF_RGB8:
        return 3;
        break;
    case PF_RGBA8:
        return 4;
        break;
    case PF_RGB16F:
        return 6;
        break;
    case PF_RGBA16F:
        return 8;
        break;
    case PF_RGB32F:
        return 12;
        break;
    case PF_RGBA32F:
        return 16;
        break;
    }
}

uint32_t s3d_create_framebuffer(uint32_t width, uint32_t height, PIXEL_FORMAT format) {
    size_t size = width * height * get_pixel_width(format);
    FBO fbo;
    fbo.width = width;
    fbo.height = height;
    fbo.size = size;
    fbo.color_address = s3d_malloc(size);
    fbo.depth_address = s3d_malloc(width * height * 4); // Always use 32 bit depth
    uint32_t id = s3d_context.fbo.used_size;
    ra_push(&s3d_context.fbo, &fbo);
    printf("Created %d x %d framebuffer with ID %d (At 0x%08x)\n", width, height, id, fbo.color_address);
    return id;
}

void s3d_clear_color() {
    FBO fbo = ((FBO *)s3d_context.fbo.buf)[s3d_context.active_fbo];
    memset(&s3d_context.vram[fbo.color_address], 0, fbo.size);
}

void s3d_clear_depth() {
    FBO fbo = ((FBO *)s3d_context.fbo.buf)[s3d_context.active_fbo];
    float *z_buffer = (float *)&s3d_context.vram[fbo.depth_address];
    for (size_t i = 0; i < fbo.width * fbo.height; i++) {
        z_buffer[i] = 1.0f;
    }
}

void s3d_depth_test(bool enable) {
    s3d_context.depth_test = enable;
}

void s3d_face_culling(bool enable) {
    s3d_context.face_culling = enable;
}

uint32_t s3d_load_ebo(void *buffer, size_t size) {
    EBO ebo;
    ebo.address = s3d_malloc(size);
    ebo.size = size;
    memcpy(&s3d_context.vram[ebo.address], buffer, size);
    uint32_t id = s3d_context.ebo.used_size;
    ra_push(&s3d_context.ebo, &ebo);
    printf("Loaded %d bytes EBO to ID %d (At 0x%08x)\n", size, id, ebo.address);
    return id;
}

uint32_t s3d_load_vbo(void *buffer, size_t size) {
    VBO vbo;
    vbo.address = s3d_malloc(size);
    vbo.size = size;
    memcpy(&s3d_context.vram[vbo.address], buffer, size);
    uint32_t id = s3d_context.vbo.used_size;
    ra_push(&s3d_context.vbo, &vbo);
    printf("Loaded %d bytes VBO to ID %d (At 0x%08x)\n", size, id, vbo.address);
    return id;
}

uint32_t s3d_bind_vao(uint32_t ebo_id, uint32_t vbo_id, uint32_t attr_size,
        uint32_t attr_stride) {
    VAO vao;
    vao.ebo_id = ebo_id;
    vao.vbo_id = vbo_id;
    vao.attribute_size = attr_size;
    vao.attribute_stride = attr_stride;
    uint32_t id = s3d_context.vao.used_size;
    ra_push(&s3d_context.vao, &vao);
    printf("Binded EBO %d VBO %d to ID %d\n", ebo_id, vbo_id, id);
    return id;
}

uint32_t s3d_load_tex(void *buffer, size_t width, size_t height, size_t channels, size_t byte_per_channel) {
    TEX tex;
    size_t size = width * height * channels * byte_per_channel;
    tex.address = s3d_malloc(size);
    tex.width = width;
    tex.height = height;
    tex.channels = channels;
    tex.byte_per_channel = byte_per_channel;
    memcpy(&s3d_context.vram[tex.address], buffer, size);
    uint32_t id = s3d_context.tex.used_size;
    ra_push(&s3d_context.tex, &tex);
    printf("Loaded %d x %d texture to ID %d (At 0x%08x)\n", width, height, id, tex.address);
    return id + 1;
}

void s3d_bind_texture(uint32_t tmu, uint32_t tex_id) {
    assert(tmu < TMU_COUNT);
    //printf("Assigning tex ID %d to TMU %d\n", tex_id, tmu);
    if (tex_id > 0) {
        s3d_context.tmu[tmu].enabled = true;
        TEX *tex = &((TEX *)s3d_context.tex.buf)[tex_id - 1];
        s3d_context.tmu[tmu].address = tex->address;
        s3d_context.tmu[tmu].width = tex->width;
        s3d_context.tmu[tmu].height = tex->height;
        s3d_context.tmu[tmu].fetch_width = tex->byte_per_channel * tex->channels;
    }
    else {
        s3d_context.tmu[tmu].enabled = false;
    }
}

void s3d_update_uniform(void *buffer, size_t size) {
    assert(size < UNIFORM_SIZE);
    memcpy(s3d_context.uniforms, buffer, size);
}

void s3d_set_varying_count(size_t count) {
    s3d_context.varying_count = count;
}

void s3d_set_pixel(FBO *fbo, int32_t x, int32_t y, uint32_t color) {
    if (x < 0) return;
    if (y < 0) return;
    if (x >= fbo->width) return;
    if (y >= fbo->height) return;
    uint32_t *buf = (uint32_t *)&s3d_context.vram[fbo->color_address];
    buf[y * fbo->width + x] = color;
}

void s3d_xline(FBO *fbo, int32_t x0, int32_t y0, int32_t x1, uint32_t color)  {
    int32_t i, xx0, xx1;

    xx0 = MIN(x0, x1);
    xx1 = MAX(x0, x1);
    for (i = xx0; i <= xx1; i++) {
        s3d_set_pixel(fbo, i, y0, color);
    }
}

void s3d_yline(FBO *fbo, int32_t x0, int32_t y0, int32_t y1, uint32_t color)  {
    int32_t i, yy0, yy1;

    yy0 = MIN(y0, y1);
    yy1 = MAX(y0, y1);
    for (i = yy0; i <= yy1; i++) {
        s3d_set_pixel(fbo, x0, i, color);
    }
}

void s3d_line(FBO *fbo, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color) {
    int32_t temp;
    int32_t dx,dy;
    int32_t s1,s2,status,i;
    int32_t Dx,Dy,sub;

    //printf("Draw line from %d, %d to %d, %d\n", x0, y0, x1, y1);

    dx = x1 - x0;
    if (dx >= 0)
        s1 = 1;
    else
        s1 = -1;
    dy = y1 - y0;
    if (dy >= 0)
        s2 = 1;
    else
        s2 = -1;

    Dx = abs(x1 - x0);
    Dy = abs(y1 - y0);
    if (Dy > Dx) {
        temp = Dx;
        Dx = Dy;
        Dy = temp;
        status = 1;
    }
    else
        status = 0;

    if(dx==0)
        s3d_yline(fbo, x0, y0, y1, color);
    if(dy==0)
        s3d_xline(fbo, x0, y0, x1, color);

    sub = 2 * Dy - Dx;
    for (i = 0; i < Dx; i++) {
        s3d_set_pixel(fbo, x0, y0, color);
        if (sub >= 0) {
            if(status == 1)
                x0 += s1;
            else
                y0 += s2;
            sub -= 2 * Dx;
        }
        if (status == 1)
            y0 += s2;
        else
            x0 += s1;
        sub += 2 * Dy;
    }
}

float float_lerp(float factor, float r1, float r2) {
    return r1 * factor + r2 * (1.f - factor);
}

void swap(int *a, int *b) {
    int tmp = *b;
    *b = *a;
    *a = tmp;
}

static int32_t int_signed_area(int32_t v0x, int32_t v0y, int32_t v1x, int32_t v1y,
        int32_t v2x, int32_t v2y) {
    int32_t vec1x = v1x - v0x;
    int32_t vec1y = v1y - v0y;
    int32_t vec2x = v2x - v0x;
    int32_t vec2y = v2y - v0y;
    return vec1x * vec2y - vec1y * vec2x;
}

void s3d_render(uint32_t vao_id) {
    // TODO: Add in RV32IF simulator
    // TODO: Implement this thing as an scheduler, like an actual GPU

    //printf("Memory usage: %d bytes\n", s3d_context.memptr);
    VAO vao = ((VAO *)s3d_context.vao.buf)[vao_id];
    VBO vbo = ((VBO *)s3d_context.vbo.buf)[vao.vbo_id];
    EBO ebo = ((EBO *)s3d_context.ebo.buf)[vao.ebo_id];
    FBO fbo = ((FBO *)s3d_context.fbo.buf)[s3d_context.active_fbo];

    uint32_t *indices = (uint32_t *)&s3d_context.vram[ebo.address];
    float *attributes = (float *)&s3d_context.vram[vbo.address];
    for (uint32_t i = 0; i < ebo.size / sizeof(uint32_t) / 3; i++) {

        //printf("Input triangle %d\n", i);
        //if ((i != 1) && (i != 0)) continue;
        //if ((i != 3)) continue;

        // START OF VS STAGE
        POST_VS_VERTEX post_vs_vertex[3] = {0};

        for (uint32_t j = 0; j < 3; j++) {
            uint32_t index = indices[i * 3 + j];
            simple_vs(
                (UNIFORM *)s3d_context.uniforms,
                &attributes[vao.attribute_stride * index],
                &post_vs_vertex[j].varying[0],
                &post_vs_vertex[j].position
            );

        }

        s3d_setup_triangle(&post_vs_vertex[0], &post_vs_vertex[1],
                &post_vs_vertex[2]);
    }

    //s3d_line(&fbo, 0, 0, 639, 479, 0xff0000ff);
}


void s3d_render_copy(uint8_t *destination) {
    FBO active_fbo = ((FBO *)s3d_context.fbo.buf)[s3d_context.active_fbo];
    uint32_t *source = &s3d_context.vram[active_fbo.color_address];
#if 0
    printf("YX ");
    for (int j = 0; j < active_fbo.width; j++) {
        putchar('0' + j % 10);
    }
    putchar('\n');
    for (int i = 0; i < active_fbo.height; i++) {
        printf("%02d ", i);
        for (int j = 0; j < active_fbo.width; j++) {
            putchar(source[i * active_fbo.width + j] ? '*' : '.');
        }
        putchar('\n');
    }
#endif
    memcpy((void *)destination, (const void *)source, active_fbo.size);
}

void s3d_delete_ebo(uint32_t ebo_id) {

}

void s3d_delete_vbo(uint32_t vbo_id) {

}

void s3d_delete_vao(uint32_t vao_id) {

}

void s3d_delete_tex(uint32_t tex_id) {

}

VEC4 s3d_tex_lookup(uint32_t tmu_id, VEC2 tex_coord) {
    TMU *tmu = &s3d_context.tmu[tmu_id];
    VEC4 result = {0.0f, 0.0f, 0.0f, 0.0f};
    if (!tmu->enabled)
        return result;
#if 1
    // TODO: Implement proper/ configurable clamping?
    tex_coord.x = fmodf(fabsf(tex_coord.x), 1.0f);
    tex_coord.y = fmodf(fabsf(tex_coord.y), 1.0f);
    /*if (tex_coord.x > 1.0f) tex_coord.x = 1.0f;
    if (tex_coord.x < 0.0f) tex_coord.x = 0.0f;
    if (tex_coord.y > 1.0f) tex_coord.y = 1.0f;
    if (tex_coord.y < 0.0f) tex_coord.y = 0.0f;*/
    //tex_coord.x = fabsf(tex_coord.x);
    //tex_coord.y = fabsf(tex_coord.y);
    int32_t texel_x = (int32_t)(tex_coord.x * tmu->width);
    //texel_x %= tmu->width;
    int32_t texel_y = (int32_t)(tex_coord.y * tmu->height);
    //texel_y %= tmu->height;
    uint32_t address = tmu->address + (texel_y * tmu->width + texel_x) * tmu->fetch_width;
    // This access is likely to be unaligned!
    uint8_t r = s3d_context.vram[address];
    uint8_t g = s3d_context.vram[address + 1];
    uint8_t b = s3d_context.vram[address + 2];
    result.x = (float)r / 255.0f;
    result.y = (float)g / 255.0f;
    result.z = (float)b / 255.0f;
    //printf("Texture lookup %.2f %.2f -> %d %d, result: %d %d %d\n", tex_coord.x, tex_coord.y, texel_x, texel_y, r, g, b);
    // TODO: Implement alpha channel
    // TODO: Implement reading from float buffer
#else
    result.x = tex_coord.x;
    result.y = tex_coord.y;
    result.z = 0;
#endif
    
    return result;
}
