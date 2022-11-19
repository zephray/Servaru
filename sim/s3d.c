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

static S3D_CONTEXT s3d_context;

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

// Helper debugging functions
#define MIN(a, b) (a < b) ? (a) : (b)
#define MAX(a, b) (a > b) ? (a) : (b)

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

static float float_lerp(float factor, float r1, float r2) {
    return r1 * factor + r2 * (1.f - factor);
}

static int32_t int_signed_area(int32_t v0x, int32_t v0y, int32_t v1x, int32_t v1y,
        int32_t v2x, int32_t v2y) {
    int32_t vec1x = v1x - v0x;
    int32_t vec1y = v1y - v0y;
    int32_t vec2x = v2x - v0x;
    int32_t vec2y = v2y - v0y;
    return vec1x * vec2y - vec1y * vec2x;
}

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

// PUT triangle related stuff in the FS TCM?
static void process_fragment(int32_t x, int32_t y, int32_t w0, int32_t w1, int32_t w2,
        POST_VS_VERTEX *v0, POST_VS_VERTEX *v1, POST_VS_VERTEX *v2) {

    //printf("Barycentric coordinates %d %d %d\n", w0, w1, w2);
    FBO fbo = ((FBO *)s3d_context.fbo.buf)[s3d_context.active_fbo];
    float *z_buffer = (float *)&s3d_context.vram[fbo.depth_address];

    float wsum = w0 + w1 + w2;
    float interpolated_z_over_w = 
            (v0->position.z * w0 + v1->position.z * w1 + v2->position.z * w2) /
            wsum;
    float frag_depth = interpolated_z_over_w; // Use for Z-test
    
    // Early Z
    if (s3d_context.early_depth_test) {
        if (!z_test(&z_buffer[y * fbo.width + x], frag_depth))
            return;
    }

    // START OF FS STAGE

    // Interpolate varyings
    VEC3 w_inverse = {v0->position.w, v1->position.w, v2->position.w};
    VEC3 baricentric_coord = {(float)w0, (float)w1, (float)w2};
    float interpolated_w_inverse = 1.0f / vec3_dot(w_inverse, baricentric_coord);
    float varying[s3d_context.varying_count];
    
    for (uint32_t i = 0; i < s3d_context.varying_count; i++) {
        //printf("Interpolating varying %d\n", i);
        VEC3 attr_over_w = {v0->varying[i], v1->varying[i], v2->varying[i]};
        //print_vec3(attr_over_w, "Attributions");
        float interpolated_attr_over_w = vec3_dot(attr_over_w, baricentric_coord);
        varying[i] = interpolated_attr_over_w * interpolated_w_inverse;
        //printf("Interpolated to %.2f\n", varying[i]);
    }

    /*if (varying[0] > 1.1f) {
        printf("V0: %.2f %.2f V1: %.2f %.2f V2: %.2f %.2f\n",
                v0->varying[0] / v0->position.w, v0->varying[1] / v0->position.w,
                v1->varying[0] / v1->position.w, v1->varying[1] / v1->position.w,
                v2->varying[0] / v2->position.w, v2->varying[1] / v2->position.w);
        printf("Interpolated to %.2f %.2f\n", varying[0], varying[1]);
    }*/

    // Fragment shading
    VEC3 frag_color;

    simple_fs(
        s3d_context.uniforms,
        varying,
        &frag_color,
        &frag_depth
    );
    // FS code could potentially write new frag_depth, late Z will use the new value

    // END OF FS STAGE

    // Late Z (if early Z is not enabled)
    if (!s3d_context.early_depth_test) {
        if (!z_test(&z_buffer[y * fbo.width + x], frag_depth))
            return;
    }

    // Color WB
    int32_t r = frag_color.x * 255.f;
    int32_t g = frag_color.y * 255.f;
    int32_t b = frag_color.z * 255.f;
    // Clamp down
    // TODO: Make this step optional?
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    uint32_t color = (b << 24) | (g << 16) | (r << 8) | 0xff;
    s3d_set_pixel(&fbo, x, y, color); 
}

typedef enum {
    STEP_NONE,
    STEP_RIGHT,
    STEP_DOWN,
    STEP_LEFT
} RAS_STEP_DIR;

typedef enum {
    ST_SCAN_RIGHT_FOR_LEFT_EDGE,
    ST_SWEEP_RIGHT,
    ST_STEPPED_RIGHT_DOWN,
    ST_SCAN_LEFT_FOR_RIGHT_EDGE,
    ST_SCAN_RIGHT_FOR_RIGHT_EDGE,
    ST_SWEEP_LEFT,
    ST_STEPPED_LEFT_DOWN,
    ST_SCAN_LEFT_FOR_LEFT_EDGE
} RAS_STATE;

typedef enum {
    COND_OUTSIDE,
    COND_INSIDE,
    COND_LEFT_EDGE,
    COND_RIGHT_EDGE
} RAS_COND;

const char *state_names[] = {
    "ST_SCAN_RIGHT_FOR_LEFT_EDGE",
    "ST_SWEEP_RIGHT",
    "ST_STEPPED_RIGHT_DOWN",
    "ST_SCAN_LEFT_FOR_RIGHT_EDGE",
    "ST_SCAN_RIGHT_FOR_RIGHT_EDGE",
    "ST_SWEEP_LEFT",
    "ST_STEPPED_LEFT_DOWN",
    "ST_SCAN_LEFT_FOR_LEFT_EDGE"
};

static void swap(int *a, int *b) {
    int tmp = *b;
    *b = *a;
    *a = tmp;
}

// QUESTION: How do I find right edge and apply bias?
static void s3d_rasterize_triangle(POST_VS_VERTEX *v0, 
        POST_VS_VERTEX *v1, POST_VS_VERTEX *v2) {
    // Rasterizer takes 2D coordinates as input
    // Generate fragments (with 2D coordinates)
    // How about let it run at a rate of ... 2 pixel per clock?
    // Note this unit would be eventually shared by multiple shaders
    RAS_STATE state = ST_SCAN_RIGHT_FOR_LEFT_EDGE;
    RAS_STATE next_state;
    RAS_STEP_DIR step_dir;
    RAS_COND cond;

    // DEBUG
    FBO fbo = ((FBO *)s3d_context.fbo.buf)[s3d_context.active_fbo];

    int32_t x0 = v0->screen_position[0];
    int32_t y0 = v0->screen_position[1];
    int32_t x1 = v1->screen_position[0];
    int32_t y1 = v1->screen_position[1];
    int32_t x2 = v2->screen_position[0];
    int32_t y2 = v2->screen_position[1];

    // Triangle setup
    // TODO: move these out of the function.
    int32_t step0_x, step0_y, step1_x, step1_y, step2_x, step2_y;
    int32_t left_edge, right_edge, upper_edge, lower_edge;
    int32_t edge0, edge1, edge2;
    int32_t x, y;

    // If it's not even an triangle...
    if ((x0 == x1) && (x1 == x2))
        return;
    if ((y0 == y1) && (y1 == y2))
        return;

    // Determine if the last 2 coordinates needs to be swapped or not...
    // Create the edge function of 0 and 1:
    step1_x = y0 - y1;
    step1_y = x1 - x0;
    edge1 = (x2 - x1) * step1_x + (y2 - y1) * step1_y;
    if (edge1 <= 0) {
        /*swap(&x1, &x2);
        swap(&y1, &y2);
        printf("Reversing...\n");*/
        return;
    }

    left_edge = MIN(x0, x1);
    left_edge = MIN(left_edge, x2);
    left_edge -= 2;
    right_edge = MAX(x0, x1);
    right_edge = MAX(right_edge, x2);
    right_edge += 2;
    upper_edge = MIN(y0, y1);
    upper_edge = MIN(upper_edge, y2);
    lower_edge = MAX(y0, y1);
    lower_edge = MAX(lower_edge, y2);

    step0_x = y2 - y0;
    step0_y = x0 - x2;
    step1_x = y0 - y1;
    step1_y = x1 - x0;
    step2_x = y1 - y2;
    step2_y = x2 - x1;

    //printf("Step 0: %d, %d\n", step0_x, step0_y);
    //printf("Step 1: %d, %d\n", step1_x, step1_y);
    //printf("Step 2: %d, %d\n", step2_x, step2_y);

    x = left_edge;
    y = upper_edge;

    //printf("Left edge %d, right edge %d\n", left_edge, right_edge);

    bool rasterizer_active = true;

#if 0
    edge0 = (0 - x0) * step0_x + (0 - y0) * step0_y;
    edge1 = (0 - x1) * step1_x + (0 - y1) * step1_y;
    edge2 = (0 - x2) * step2_x + (0 - y2) * step2_y;

    for (int y = 0; y < 480; y++) {
        int32_t e0 = edge0;
        int32_t e1 = edge1;
        int32_t e2 = edge2;
        for (int x = 0; x < 640; x++) {
            /*uint32_t color = 0x000000ff;
            if (e0 >= 0) color |= 0x3f000000;
            if (e1 >= 0) color |= 0x003f0000;
            if (e2 >= 0) color |= 0x00003f00;
            if (color == 0x3f3f3fff) color = 0xffffffff;
            s3d_set_pixel(&fbo, j, i, color);*/
            if ((e0 >= 0) && (e1 >= 0) && (e2 >= 0))
                process_fragment(x, y, edge2, edge0, edge1, v0, v1, v2);
            e0 += step0_x;
            e1 += step1_x;
            e2 += step2_x;
        }
        edge0 += step0_y;
        edge1 += step1_y;
        edge2 += step2_y;
    }
#else 

    // In extreme case, value as large as X_RES ^ 2 + Y_RES ^ 2 could be stored
    // into this variable.
    // For 640*480: 640*640+480*480= 640000
    // For 4096*4096: 4K^2 * 2 = 32M
    // uint32_t should be more than sufficient.
    edge0 = (x - x0) * step0_x + (y - y0) * step0_y;
    edge1 = (x - x1) * step1_x + (y - y1) * step1_y;
    edge2 = (x - x2) * step2_x + (y - y2) * step2_y;

    uint32_t loop_counter = 0;

    while (rasterizer_active) {
        // Evaluate edge functions
        bool inside = ((edge0 >= 0) && (edge1 >= 0) && (edge2 >= 0));
        if (inside)
            cond = COND_INSIDE;
        else if (x == left_edge)
            cond = COND_LEFT_EDGE;
        else if (x == right_edge)
            cond = COND_RIGHT_EDGE;
        else
            cond = COND_OUTSIDE;
        
        // This pixel valid is a mask: if it's false, the output is disgarded.
        // It doesn't necessarily means it's currently producing a valid pixel.
        bool pixel_valid;
        
        switch (state) {
        case ST_SCAN_RIGHT_FOR_LEFT_EDGE:
            switch (cond) {
            case COND_OUTSIDE:      step_dir = STEP_RIGHT;  pixel_valid = false; next_state = ST_SCAN_RIGHT_FOR_LEFT_EDGE; break;
            case COND_INSIDE:       step_dir = STEP_RIGHT;  pixel_valid = true;  next_state = ST_SWEEP_RIGHT; break;
            case COND_LEFT_EDGE:    step_dir = STEP_RIGHT;  pixel_valid = true;  next_state = ST_SCAN_RIGHT_FOR_LEFT_EDGE; break;
            case COND_RIGHT_EDGE:   step_dir = STEP_DOWN;   pixel_valid = false; next_state = ST_STEPPED_RIGHT_DOWN; break;
            }
            break;
        case ST_SWEEP_RIGHT:
            switch (cond) {
            case COND_OUTSIDE:      step_dir = STEP_DOWN;   pixel_valid = false; next_state = ST_STEPPED_RIGHT_DOWN; break;
            case COND_INSIDE:       step_dir = STEP_RIGHT;  pixel_valid = true;  next_state = ST_SWEEP_RIGHT; break;
            case COND_LEFT_EDGE:    step_dir = STEP_RIGHT;  pixel_valid = true;  next_state = ST_SWEEP_RIGHT; break;
            case COND_RIGHT_EDGE:   step_dir = STEP_DOWN;   pixel_valid = false; next_state = ST_SWEEP_LEFT; break;
            }
            break;
        case ST_STEPPED_RIGHT_DOWN:
            switch (cond) {
            case COND_OUTSIDE:      step_dir = STEP_LEFT;   pixel_valid = false; next_state = ST_SCAN_LEFT_FOR_RIGHT_EDGE; break;
            case COND_INSIDE:       step_dir = STEP_RIGHT;  pixel_valid = false; next_state = ST_SCAN_RIGHT_FOR_RIGHT_EDGE; break;
            case COND_LEFT_EDGE:    assert(0); break;
            case COND_RIGHT_EDGE:   step_dir = STEP_LEFT;   pixel_valid = false; next_state = ST_SCAN_LEFT_FOR_RIGHT_EDGE; break;
            }
            break;
        case ST_SCAN_LEFT_FOR_RIGHT_EDGE:
            switch (cond) {
            case COND_OUTSIDE:      step_dir = STEP_LEFT;   pixel_valid = false; next_state = ST_SCAN_LEFT_FOR_RIGHT_EDGE; break;
            case COND_INSIDE:       step_dir = STEP_LEFT;   pixel_valid = true;  next_state = ST_SWEEP_LEFT; break;
            case COND_LEFT_EDGE:    step_dir = STEP_RIGHT;  pixel_valid = false; next_state = ST_SCAN_RIGHT_FOR_LEFT_EDGE; break;
            case COND_RIGHT_EDGE:   assert(0); break;
            }
            break;
        case ST_SCAN_RIGHT_FOR_RIGHT_EDGE:
            switch (cond) {
            case COND_OUTSIDE:      step_dir = STEP_LEFT;   pixel_valid = false; next_state = ST_SWEEP_LEFT; break;
            case COND_INSIDE:       step_dir = STEP_RIGHT;  pixel_valid = false; next_state = ST_SCAN_RIGHT_FOR_RIGHT_EDGE; break;
            case COND_LEFT_EDGE:    assert(0); break;
            case COND_RIGHT_EDGE:   step_dir = STEP_LEFT;   pixel_valid = true;  next_state = ST_SWEEP_LEFT; break;
            }
            break;
        case ST_SWEEP_LEFT:
            switch (cond) {
            case COND_OUTSIDE:      step_dir = STEP_DOWN;   pixel_valid = false; next_state = ST_STEPPED_LEFT_DOWN; break;
            case COND_INSIDE:       step_dir = STEP_LEFT;   pixel_valid = true;  next_state = ST_SWEEP_LEFT; break;
            case COND_LEFT_EDGE:    step_dir = STEP_DOWN;   pixel_valid = true;  next_state = ST_SWEEP_RIGHT; break;
            case COND_RIGHT_EDGE:   step_dir = STEP_LEFT;   pixel_valid = true;  next_state = ST_SWEEP_LEFT; break;
            }
            break;
        case ST_STEPPED_LEFT_DOWN:
            switch (cond) {
            case COND_OUTSIDE:      step_dir = STEP_RIGHT;  pixel_valid = false; next_state = ST_SCAN_RIGHT_FOR_LEFT_EDGE; break;
            case COND_INSIDE:       step_dir = STEP_LEFT;   pixel_valid = false; next_state = ST_SCAN_LEFT_FOR_LEFT_EDGE; break;
            case COND_LEFT_EDGE:    assert(0); break;
            case COND_RIGHT_EDGE:   assert(0); break;
            }
            break;
        case ST_SCAN_LEFT_FOR_LEFT_EDGE:
            switch (cond) {
            case COND_OUTSIDE:      step_dir = STEP_RIGHT;  pixel_valid = false; next_state = ST_SWEEP_RIGHT; break;
            case COND_INSIDE:       step_dir = STEP_LEFT;   pixel_valid = false; next_state = ST_SCAN_LEFT_FOR_LEFT_EDGE; break;
            case COND_LEFT_EDGE:    step_dir = STEP_RIGHT;  pixel_valid = true;  next_state = ST_SCAN_RIGHT_FOR_LEFT_EDGE; break;
            case COND_RIGHT_EDGE:   step_dir = STEP_LEFT;   pixel_valid = false; next_state = ST_SCAN_LEFT_FOR_LEFT_EDGE; break;
            }
            break;
        }

        //printf("X %d, Y %d, state %s, next_state %s\n", x, y, state_names[state], state_names[next_state]);

#if 0
                        // B G R A
        uint32_t color = 0x000000ff;
        if (edge0 >= 0) color |= 0x3f000000;
        if (edge1 >= 0) color |= 0x003f0000;
        if (edge2 >= 0) color |= 0x00003f00;
        s3d_set_pixel(&fbo, x, y, color);
#endif

        // Processed pixel:
        if (inside && pixel_valid) {
            //s3d_set_pixel(&fbo, x, y, 0xffffffff);
            //printf("Valid pixel %d %d %d %d %d\n", x, y, edge0, edge1, edge2);
            process_fragment(x, y, edge2, edge0, edge1, v0, v1, v2);
        }
        else {
            //printf("Invalid pixel %d %d %d\n", edge0, edge1, edge2);
        }


        // Stepping based on the direction
        switch (step_dir) {
        case STEP_DOWN:  edge0 += step0_y; edge1 += step1_y; edge2 += step2_y; y += 1; break;
        case STEP_LEFT:  edge0 -= step0_x; edge1 -= step1_x; edge2 -= step2_x; x -= 1; break;
        case STEP_RIGHT: edge0 += step0_x; edge1 += step1_x; edge2 += step2_x; x += 1; break;
        default: break; // Ignore step_none
        }

        // Exit
        if (y == lower_edge)
            rasterizer_active = false;

        state = next_state;

        loop_counter++;
        if (loop_counter > 640*480) {
            printf("Infinite loop detected!\n");
            print_vec4(v0->position, "V0 position");
            print_vec4(v1->position, "V1 position");
            print_vec4(v2->position, "V2 position");
            return;
        }
    }
#endif

    //printf("Rasterization done.\n");
}

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

static POST_VS_VERTEX get_intersection(VEC4 *edge, POST_VS_VERTEX *v0, POST_VS_VERTEX *v1, float w_bias) {
    float dp = vec4_dot_w_bias(&v0->position, edge, w_bias);
    float dp_prev = vec4_dot_w_bias(&v1->position, edge, w_bias);
    float factor = dp_prev / (dp_prev - dp);
    POST_VS_VERTEX result;
    result.position = vec4_lerp(factor, v0->position, v1->position);
    for (int i = 0; i < s3d_context.varying_count; i++)
        result.varying[i] = float_lerp(factor, v0->varying[i], v1->varying[i]);
    return result;
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
        POST_VS_VERTEX position_a[9] = {0};
        POST_VS_VERTEX position_b[9] = {0};
        
        for (uint32_t j = 0; j < 3; j++) {
            uint32_t index = indices[i * 3 + j];
            simple_vs(
                (UNIFORM *)s3d_context.uniforms,
                &attributes[vao.attribute_stride * index],
                &position_a[j].varying[0],
                &position_a[j].position
            );

        }


#if 0   // Culling before clipping doesn't work. But is there any workaround?
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
            // Interpolationg (affine/ perspective correct)

            //break; // DEBUG
        }
        // Culling away clockwise triangles

        //if (i == 1) break; // DEBUG
    }

    //s3d_line(&fbo, 0, 0, 639, 479, 0xff0000ff);
}


void s3d_render_copy(uint8_t *destination) {
    FBO active_fbo = ((FBO *)s3d_context.fbo.buf)[s3d_context.active_fbo];
    uint8_t *source = &s3d_context.vram[active_fbo.color_address];
    memcpy(destination, source, active_fbo.size);
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
