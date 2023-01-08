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

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

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

uint32_t s3d_map_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return 0xff000000ul | (b << 16) | (g << 8) | (r);
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

static void s3d_mipmap_put_pixel(uint8_t *texture, size_t width, size_t level, size_t x, size_t y, uint32_t r, uint32_t g, uint32_t b) {
    size_t offset = 1u << level;
    texture[y * width + (offset + x)] = r;
    texture[(offset + y) * width + x] = g;
    texture[(offset + y) * width + (offset + x)] = b;
}

uint8_t *s3d_create_mipmap(uint8_t *image, size_t side, size_t level) {
    // At this point, the width should be equal to 2^^k (height = width)
    uint8_t *temp = malloc(side * side * 3);
    uint8_t *target = malloc(side * side * 4);
    for (int l = level; l >= 0; l--) {
        // Resize to target level
        uint32_t level_side = 1ul << l;
        stbir_resize_uint8(image, side, side, 0, temp, level_side, level_side, 0, 3);
        // Fill in mipmap
        for (size_t y = 0; y < level_side; y++) {
            for (size_t x = 0; x < level_side; x++) {
                uint8_t *pixel = &temp[(y * level_side + x) * 3];
                uint8_t r = *pixel++;
                uint8_t g = *pixel++;
                uint8_t b = *pixel++;
                s3d_mipmap_put_pixel(target, side * 2, l, x, y, r, g, b);
            }
        }
    }
    free(temp);
    return target;
}

uint32_t s3d_load_tex(void *buffer, size_t width, size_t height, size_t channels, size_t byte_per_channel) {
    TEX tex;
    // Only support 8bpc RGB format now!
    assert(byte_per_channel == 1);
    if (channels == 4) {
        // Drop A channel and try again
        uint32_t texid;
        uint8_t *temp = malloc(width * height * 3);
        uint8_t *src = buffer;
        uint8_t *dst = temp;
        for (int i = 0; i < width * height; i++) {
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = *src++;
            src++;
        }
        texid = s3d_load_tex(temp, width, height, 3, 1);
        free(temp);
        return texid;
    }
    assert(channels == 3);

    // Determine the actual allocated size
    float scale_x = (float)MAX_TEXTURE_SIZE / (float)width;
    float scale_y = (float)MAX_TEXTURE_SIZE / (float)height;
    float scale = fminf(scale_x, scale_y);
    uint32_t target_width = width;
    uint32_t target_height = height;
    if (scale < 1.0f) {
        printf("Scale: %f\n", scale);
        target_width *= scale;
        target_height *= scale;
    }
    // TODO: properly handle non 1:1 texture?
    uint32_t side = MAX(target_height, target_width);
    // Level 0 means 1x1, level 1 is 2x2, etc.
    size_t level = (size_t)(ceilf(logf((float)side) / logf(2.0f)));
    target_width = 1ul << level;
    target_height = 1ul << level;
    // Convert to format accepted by s3d_create_mipmap
    uint8_t *temp = malloc(target_height * target_width * 3);
    stbir_resize_uint8(buffer, width, height, 0, temp, target_width, target_height, 0, 3);

    uint8_t *mipmap = s3d_create_mipmap(temp, target_width, level);
    free(temp);
    size_t size = target_width * target_height * 4;
    tex.address = s3d_malloc(size);
    tex.width = target_width;
    tex.height = target_height;
    tex.mipmap_levels = level;

    memcpy(&s3d_context.vram[tex.address], mipmap, size);
    free(mipmap);
    uint32_t id = s3d_context.tex.used_size;
    ra_push(&s3d_context.tex, &tex);
    printf("Loaded %d x %d (from %d x %d) texture to ID %d (At 0x%08x)\n",
            target_width, target_height, width, height, id, tex.address);
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
        s3d_context.tmu[tmu].mipmap_levels = tex->mipmap_levels;
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

VEC3 vec3_lerp(float factor, VEC3 r1, VEC3 r2) {
    VEC3 result;
    result.x = float_lerp(factor, r1.x, r2.x);
    result.y = float_lerp(factor, r1.y, r2.y);
    result.z = float_lerp(factor, r1.z, r2.z);
    return result;
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

#if 1
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
#endif

#if 0
    // Check texture mapping
    // TMU tmu = s3d_context.tmu[0];
    // if (!tmu.enabled)
    //     return;
    // FBO active_fbo = ((FBO *)s3d_context.fbo.buf)[s3d_context.active_fbo];
    // uint32_t width = MIN(active_fbo.width, tmu.width * 2);
    // uint32_t height = MIN(active_fbo.height, tmu.height * 2);
    // for (int y = 0; y < height; y++) {
    //     for (int x = 0; x < width; x++) {
    //         uint32_t address = tmu.address + (y * tmu.width * 2 + x);
    //         uint32_t r = s3d_context.vram[address];
    //         uint32_t color = s3d_map_rgb(r, r, r);
    //         s3d_set_pixel(&active_fbo, x, y, color); 
    //     }
    // }
    TMU tmu = s3d_context.tmu[0];
    if (!tmu.enabled)
        return;
    FBO active_fbo = ((FBO *)s3d_context.fbo.buf)[s3d_context.active_fbo];
    uint32_t width = active_fbo.width;
    uint32_t height = active_fbo.height;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            VEC2 tex_coord = {(float)x / width, (float)y / height};
            VEC4 color = s3d_tex_lookup(0, 0, tex_coord);
            uint32_t c = s3d_map_rgb((int)(color.x * 255.0f),
                    (int)(color.y * 255.0f), (int)(color.z * 255.0f));
            s3d_set_pixel(&active_fbo, x, y, c); 
        }
    }
#endif

    //s3d_line(&fbo, 0, 0, 639, 479, 0xff0000ff);
}

int min_level = 100;
int max_level = -1;

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
    printf("Mipmap [%d, %d]\n", min_level, max_level);
    min_level = 100;
    max_level = -1;

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
