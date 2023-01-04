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

const char *cond_names[] = {
    "COND_OUTSIDE",
    "COND_INSIDE",
    "COND_LEFT_EDGE",
    "COND_RIGHT_EDGE"
};


// QUESTION: How do I find right edge and apply bias?
void s3d_rasterize_triangle(POST_VS_VERTEX *v0, 
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
    int32_t edge0[4], edge1[4], edge2[4];
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
    edge1[0] = (x2 - x1) * step1_x + (y2 - y1) * step1_y;
    if (edge1[0] <= 0) {
        /*swap(&x1, &x2);
        swap(&y1, &y2);
        printf("Reversing...\n");*/
        return;
    }

    left_edge = MIN(x0, x1);
    left_edge = MIN(left_edge, x2);
    left_edge = (left_edge - 2) / 2 * 2;
    right_edge = MAX(x0, x1);
    right_edge = MAX(right_edge, x2);
    right_edge = (right_edge + 2) / 2 * 2;
    upper_edge = MIN(y0, y1);
    upper_edge = MIN(upper_edge, y2);
    upper_edge = upper_edge / 2 * 2;
    lower_edge = MAX(y0, y1);
    lower_edge = MAX(lower_edge, y2);
    lower_edge = (lower_edge + 2) / 2 * 2;

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
    edge0[0] = (x - x0) * step0_x + (y - y0) * step0_y;
    edge1[0] = (x - x1) * step1_x + (y - y1) * step1_y;
    edge2[0] = (x - x2) * step2_x + (y - y2) * step2_y;
    // 0 1 EDGE
    // 2 3 FUNC

    uint32_t loop_counter = 0;

    while (rasterizer_active) {
        // Update additional edge funcs
        // TODO: Optimize for hardware
        edge0[1] = edge0[0] + step0_x;
        edge1[1] = edge1[0] + step1_x;
        edge2[1] = edge2[0] + step2_x;
        edge0[2] = edge0[0] + step0_y;
        edge1[2] = edge1[0] + step1_y;
        edge2[2] = edge2[0] + step2_y;
        edge0[3] = edge0[2] + step0_x;
        edge1[3] = edge1[2] + step1_x;
        edge2[3] = edge2[2] + step2_x;

        // Evaluate edge functions
        bool inside[4];
        for (int i = 0; i < 4; i++) {
            inside[i] = ((edge0[i] >= 0) && (edge1[i] >= 0) && (edge2[i] >= 0));
        }
        bool any_inside = inside[0] | inside[1] | inside[2] | inside[3];

        if (any_inside)
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
            case COND_RIGHT_EDGE:   step_dir = STEP_DOWN;   pixel_valid = false; next_state = ST_STEPPED_RIGHT_DOWN; break;
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
            case COND_LEFT_EDGE:    step_dir = STEP_DOWN;   pixel_valid = true;  next_state = ST_STEPPED_LEFT_DOWN; break;
            case COND_RIGHT_EDGE:   step_dir = STEP_LEFT;   pixel_valid = true;  next_state = ST_SWEEP_LEFT; break;
            }
            break;
        case ST_STEPPED_LEFT_DOWN:
            switch (cond) {
            case COND_OUTSIDE:      step_dir = STEP_RIGHT;  pixel_valid = false; next_state = ST_SCAN_RIGHT_FOR_LEFT_EDGE; break;
            case COND_INSIDE:       step_dir = STEP_LEFT;   pixel_valid = false; next_state = ST_SCAN_LEFT_FOR_LEFT_EDGE; break;
            case COND_LEFT_EDGE:    step_dir = STEP_RIGHT;  pixel_valid = false; next_state = ST_SCAN_RIGHT_FOR_LEFT_EDGE; break;
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

        //printf("X %d, Y %d, cond %s, state %s, next_state %s\n", x, y, cond_names[cond], state_names[state], state_names[next_state]);

#if 0
                        // B G R A
        uint32_t color = 0x000000ff;
        if (edge0 >= 0) color |= 0x3f000000;
        if (edge1 >= 0) color |= 0x003f0000;
        if (edge2 >= 0) color |= 0x00003f00;
        s3d_set_pixel(&fbo, x, y, color);
#endif

        // Processed pixel:
        if (any_inside && pixel_valid) {
            //s3d_set_pixel(&fbo, x, y, 0xffffffff);
            //printf("Valid pixel %d %d %d %d %d\n", x, y, edge0, edge1, edge2);
            //s3d_process_fragment(x, y, edge2, edge0, edge1, v0, v1, v2);
            s3d_process_fragments(inside, x, y, edge2, edge0, edge1, v0, v1, v2);
        }

        // Stepping based on the direction
        switch (step_dir) {
        case STEP_DOWN:  edge0[0] += step0_y * 2; edge1[0] += step1_y * 2; edge2[0] += step2_y * 2; y += 2; break;
        case STEP_LEFT:  edge0[0] -= step0_x * 2; edge1[0] -= step1_x * 2; edge2[0] -= step2_x * 2; x -= 2; break;
        case STEP_RIGHT: edge0[0] += step0_x * 2; edge1[0] += step1_x * 2; edge2[0] += step2_x * 2; x += 2; break;
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