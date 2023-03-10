//
// Servaru
// Copyright 2023 Wenting Zhang, Anhang Li
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
#include "../defs.h"
#include "tmesh.hh"
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <assert.h>
#include <SDL.h>
#include "vecmath.h"

#define TMESH_SUBDIV 40
#define MIN(a, b) (a < b) ? (a) : (b)
#define MAX(a, b) (a > b) ? (a) : (b)

void print_triangle(std::array<std::array<int,2>, 3> t) {
    std::cout << "< [" <<  t[0][0] << ',' <<t[0][1];
    std::cout << "] [" <<  t[1][0] << ',' <<t[1][1];
    std::cout << "] [" <<  t[2][0] << ',' <<t[2][1] << "] >" << std::endl;
    return;
};

typedef struct {
    // TODO: Keep these as a union... if that ever matters
    // VEC4 position; // Not kept after rasterization step, only for clipping
    int32_t screen_position[4];
    // float varying[MAX_VARYING - 4];
} POST_VS_VERTEX;

void rasterize(POST_VS_VERTEX* v0, POST_VS_VERTEX* v1, POST_VS_VERTEX* v2);

SDL_Surface* semu;  // Frame Buffer
SDL_Surface* sscr;  // Screen (scaled)
SDL_Rect rsrc(0,0,EMU_WIDTH, EMU_HEIGHT);
SDL_Rect rdst(0,0,SCREEN_WIDTH, SCREEN_HEIGHT);

uint32_t color_hack = 0x7F7F7FFF; // Ugh.. this is bad!
int main(int argc, char** argv) {
    SDL_Event event;
    // Initialize SDL Session
    if(SDL_Init(SDL_INIT_EVERYTHING)<0){
        exit(1);
    }
    sscr = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE);
    semu = SDL_CreateRGBSurface(SDL_SWSURFACE, EMU_WIDTH, EMU_HEIGHT, 32, 0, 0, 0, 0);
    assert(sscr);
    assert(semu);
    
    std::cout << "FB Size: " << EMU_WIDTH << EMU_HEIGHT << std::endl;
    std::cout << "Running Rasterizer Testbench..." << std::endl << std::flush;
    // Create randomized triangle mesh
    std::vector<std::array<std::array<int,2>, 3>> triangles = \
        CreateTriangleMesh(EMU_WIDTH, EMU_HEIGHT,TMESH_SUBDIV);

    SDL_FillRect(sscr,&rdst,0x000000FF);
    SDL_FillRect(semu,&rsrc,0x000000FF);
    SDL_SoftStretch(semu, &rsrc, sscr, &rdst);
    SDL_PollEvent(&event);
    SDL_Flip(sscr);

    // Rasterize!
    int n_trig = triangles.size();
    int i_trig = 0;
    for (auto& t: triangles) {
        uint8_t gray = 255*((float)i_trig/(float)n_trig/2.f)+128;
        color_hack = gray<<24|gray<<16|gray<<8|0xFF;
        i_trig+=1;
        POST_VS_VERTEX p0 = {
            {t[0][0],t[0][1]}
        };

        POST_VS_VERTEX p1 = {
            {t[1][0],t[1][1]}
        };

        POST_VS_VERTEX p2 = {
            {t[2][0],t[2][1]}
        };

        print_triangle(t);
        rasterize(&p0, &p1, &p2);
        std::cout << std::flush;
        ((uint32_t*)semu->pixels)[10]=0xFF00FFFF;
        SDL_PollEvent(&event);
        SDL_SoftStretch(semu, &rsrc, sscr, &rdst);
        SDL_Flip(sscr);
        SDL_Delay(10);
    }
    

    float time_delta = 0.0f;
    int last_ticks = SDL_GetTicks();

    int exit = 0;

    while (!exit) {
        int cur_ticks = SDL_GetTicks();
        time_delta -= cur_ticks - last_ticks; // Actual ticks passed since last iteration
        time_delta += 1000.0f / 30.f; // Time allocated for this iteration
        last_ticks = cur_ticks;

        // S3D internally uses SDL for window creation, directly use SDL functions for events
        
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                exit = true;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    exit = true;
                    break;
                default:
                    break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    exit = true;
                    break;
                default:
                    break;
                }
                break;
            }
        }
        // Simple pipeline
        if (time_delta > 0) {
            SDL_SoftStretch(semu, &rsrc, sscr, &rdst);
            SDL_Flip(sscr);
        }
        else {
            // skip frame
        }

        int time_to_wait = time_delta - (SDL_GetTicks() - last_ticks);
        if (time_to_wait > 0)
            SDL_Delay(time_to_wait);
	}
    return 0;
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

const char *cond_names[] = {
    "COND_OUTSIDE",
    "COND_INSIDE",
    "COND_LEFT_EDGE",
    "COND_RIGHT_EDGE"
};

void rasterize(POST_VS_VERTEX* v0, POST_VS_VERTEX* v1, POST_VS_VERTEX* v2) {
    // Rasterizer takes 2D coordinates as input
    // Generate fragments (with 2D coordinates)
    // How about let it run at a rate of ... 2 pixel per clock?
    // Note this unit would be eventually shared by multiple shaders
   // Rasterizer takes 2D coordinates as input
    // Generate fragments (with 2D coordinates)
    // How about let it run at a rate of ... 2 pixel per clock?
    // Note this unit would be eventually shared by multiple shaders
    RAS_STATE state = ST_SCAN_RIGHT_FOR_LEFT_EDGE;
    RAS_STATE next_state;
    RAS_STEP_DIR step_dir;
    RAS_COND cond;

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
    left_edge  -= 2;
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

    x = left_edge;
    y = upper_edge;


    bool rasterizer_active = true;

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

        // Processed pixel:
        if (inside && pixel_valid) {
            //s3d_set_pixel(&fbo, x, y, 0xffffffff);
            //printf("Valid pixel %d %d %d %d %d\n", x, y, edge0, edge1, edge2);
            // process_fragment(x, y, edge2, edge0, edge1, v0, v1, v2);
            uint32_t color;
            uint32_t* pix_addr = ((uint32_t*)(semu->pixels))+(y*EMU_WIDTH+x);
            if(*pix_addr & 0xFFFFFF00 == 0) {
                color = 0x0000FFFF;
            } else {
                color = color_hack;
            }
            *pix_addr = color;
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
            return;
        }
    }

    printf("Rasterization done.\n");
}