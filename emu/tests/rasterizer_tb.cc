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
// #include "../defs.h"
#include "tmesh.hh"
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <assert.h>
#include <SDL.h>
#include <SDL/SDL_draw.h>
#include "vecmath.h"

#include "fixed.hpp"
#include "fixed_math.hpp"
#include "fixed_ios.hpp"

#define EMU_WIDTH       256
#define EMU_HEIGHT      128
#define EMU_SCALE       2
#define SCREEN_WIDTH    (EMU_WIDTH*EMU_SCALE)
#define SCREEN_HEIGHT   (EMU_HEIGHT*EMU_SCALE)
#define Z_NEAR          0.1f
#define Z_FAR           5000.0f
#define PERSPECTIVE_FOV 45.f
#define ORTHO_SIZE      100.f
#define FPS_CAP         60.f


#define TMESH_SUBDIV 40
#define MIN(a, b) (a < b) ? (a) : (b)
#define MAX(a, b) (a > b) ? (a) : (b)

/*
int asr(int value, int amount) {
    #define USES_ARITHMETIC_SHR(TYPE) ((TYPE)(-1) >> 1 == (TYPE)(-1))
    return !USES_ARITHMETIC_SHR(int) && value < 0 ? ~(~value >> amount) : value >> amount ;
}
*/
int asr(int value, int amount) {
    return value < 0 ? ~(~value >> amount) : value >> amount ;
}


void print_triangle(std::array<std::array<fpm::fixed_24_8,2>, 3> t) {
    std::cout << "< [" <<  t[0][0] << ',' <<t[0][1];
    std::cout << "] [" <<  t[1][0] << ',' <<t[1][1];
    std::cout << "] [" <<  t[2][0] << ',' <<t[2][1] << "] >" << std::endl;
    return;
};

typedef union {
    // VEC4 position; // Not kept after rasterization step, only for clipping
    fpm::fixed_24_8 screen_position[4];
    // uint32_t        pixel_position[4];
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
    
    std::vector<std::array<std::array<int,2>, 3>> triangle_coord = \
        CreateTriangleMesh(EMU_WIDTH, EMU_HEIGHT,TMESH_SUBDIV);
    

    std::vector<std::array<std::array<fpm::fixed_24_8,2>, 3>>   triangles_fixed;
    // std::vector<std::array<std::array<int,2>, 3>>   triangles;
    // std::vector<std::array<std::array<float,2>, 3>> triangle_coord;
    std::vector<uint32_t>                           triangle_color;

    // triangle_coord.push_back({{{{0.5,0.5}},{{1.5,3.5}},{{5.5,1.5}}}});
    // triangle_color.push_back(0x00FF00FF);

    for (auto& t: triangle_coord){
        // Converting float triangle coordinates to fixed point
        triangles_fixed.push_back({{
            {{static_cast<fpm::fixed_24_8>(t[0][0]),static_cast<fpm::fixed_24_8>(t[0][1])}},
            {{static_cast<fpm::fixed_24_8>(t[1][0]),static_cast<fpm::fixed_24_8>(t[1][1])}},
            {{static_cast<fpm::fixed_24_8>(t[2][0]),static_cast<fpm::fixed_24_8>(t[2][1])}}
        }});
    }

    SDL_FillRect(sscr,&rdst,0x000000FF);
    SDL_FillRect(semu,&rsrc,0x000000FF);
    SDL_SoftStretch(semu, &rsrc, sscr, &rdst);
    SDL_PollEvent(&event);
    SDL_Flip(sscr);

    // Rasterize!
    int n_trig = triangles_fixed.size();
    int i_trig = 0;
    for (auto& t: triangles_fixed) {
        uint8_t gray = 255*((float)i_trig/(float)n_trig/2.f)+128;
        color_hack = gray<<24|gray<<16|gray<<8|0xFF;
        i_trig+=1;
        POST_VS_VERTEX p0 = {{t[0][0],t[0][1]}};
        POST_VS_VERTEX p1 = {{t[1][0],t[1][1]}};
        POST_VS_VERTEX p2 = {{t[2][0],t[2][1]}};

        print_triangle(t);
        rasterize(&p0, &p1, &p2);
        std::cout << std::flush;
        ((uint32_t*)semu->pixels)[10]=0xFF00FFFF;
        SDL_PollEvent(&event);
        SDL_SoftStretch(semu, &rsrc, sscr, &rdst);
        SDL_Flip(sscr);
        SDL_Delay(100);
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


void rasterize(POST_VS_VERTEX* v0, POST_VS_VERTEX* v1, POST_VS_VERTEX* v2) {

    fpm::fixed_24_8 x0 = v0->screen_position[0];
    fpm::fixed_24_8 y0 = v0->screen_position[1];
    fpm::fixed_24_8 x1 = v1->screen_position[0];
    fpm::fixed_24_8 y1 = v1->screen_position[1];
    fpm::fixed_24_8 x2 = v2->screen_position[0];
    fpm::fixed_24_8 y2 = v2->screen_position[1];

    // Triangle setup
    fpm::fixed_24_8 step0_x, step0_y, step1_x, step1_y, step2_x, step2_y;   // Pixel Steps
    int32_t         left_edge, right_edge, upper_edge, lower_edge;          // Pixel bounds
    fpm::fixed_24_8 edge0, edge1, edge2;            // Edge functions
    int32_t         x, y;                           // Pixel Raster Coordinates

    // If it's not even an triangle...
    if ((x0 == x1) && (x1 == x2))   return;
    if ((y0 == y1) && (y1 == y2))   return;

    // Determine if the last 2 coordinates needs to be swapped or not...
    // Create the edge function of 0 and 1:
    
    // step1_x = y0 - y1;
    // step1_y = x1 - x0;
    // edge1 = (x2 - x1) * step1_x + (y2 - y1) * step1_y;
    // if (edge1 <= 0) {
    //     // swap(&x1, &x2);
    //     // swap(&y1, &y2);
    //     // printf("Reversing...\n");
    //     // return;
    // }

    // Bonding boxes
    left_edge  = (int)floor(std::min({x0, x1, x2}));
    right_edge = (int) ceil(std::max({x0, x1, x2}));
    upper_edge = (int)floor(std::min({y0, y1, y2}));
    lower_edge = (int) ceil(std::max({y0, y1, y2}));

    // Step vectors
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
#define EVAL_EDGES do {\
    edge0 = (x - x0) * step0_x + (y - y0) * step0_y; \
    edge1 = (x - x1) * step1_x + (y - y1) * step1_y; \
    edge2 = (x - x2) * step2_x + (y - y2) * step2_y; \
} while(0)

#define IMPL0

#ifdef IMPL0
    for (x=left_edge;x<right_edge;x++){
        for (y=upper_edge;y<lower_edge;y++){
            EVAL_EDGES;
            int e0 = static_cast<int>(edge0);
            int e1 = static_cast<int>(edge1);
            int e2 = static_cast<int>(edge2);
            if ((e0 >= 0) && (e1 >= 0) && (e2 >= 0)){
                // std::cout << "Point: " << x << ',' << y << std::endl << std::flush;
                uint32_t color;
                uint32_t* pix_addr = ((uint32_t*)(semu->pixels))+(y*EMU_WIDTH+x);
                if(*pix_addr != 0x000000FF) {
                    color = 0x0000FFFF;
                } else {
                    color = color_hack;
                }
                *pix_addr = color;
            }
        }
    }
#endif
#ifdef IMPL1
    bool hysteresis = false;
    for (x=left_edge;x<right_edge+2;x++){
        for (y=upper_edge;y<lower_edge+2;y++){
            EVAL_EDGES;
            int e0 = static_cast<int>(edge0);
            int e1 = static_cast<int>(edge1);
            int e2 = static_cast<int>(edge2);
            bool in_area = ((e0 >= 0) && (e1 >= 0) && (e2 >= 0));
            if (in_area||hysteresis){
                // std::cout << "Point: " << x << ',' << y << std::endl << std::flush;
                uint32_t color;
                uint32_t* pix_addr = ((uint32_t*)(semu->pixels))+(y*EMU_WIDTH+x);
                if(*pix_addr != 0x000000FF) {
                    color = 0x0000FFFF;
                } else {
                    color = color_hack;
                }
                *pix_addr = color;
            }
            if(hysteresis)
                hysteresis = false;
            if(in_area)
                hysteresis = true;
        }
        hysteresis = false;
    }
#endif



}

