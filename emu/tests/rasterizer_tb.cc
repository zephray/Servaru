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
#include <algorithm>    // std::random_shuffle
#include <thread>
#include <iomanip>
#include <assert.h>
#include <SDL.h>
#include <SDL/SDL_draw.h>
#include "vecmath.h"

#include "fixed.hpp"
#include "fixed_math.hpp"
#include "fixed_ios.hpp"

// std::mt19937 generator {114514};

#define TEST_MESH

#ifdef TEST_MESH
    #define EMU_WIDTH       640
    #define EMU_HEIGHT      480
    #define EMU_SCALE       2
    #define SCREEN_WIDTH    (EMU_WIDTH*EMU_SCALE)
    #define SCREEN_HEIGHT   (EMU_HEIGHT*EMU_SCALE)
    #define TMESH_SUBDIV 40
#endif
#ifdef TEST_DX10
    #define EMU_WIDTH       16
    #define EMU_HEIGHT      8
    #define EMU_SCALE       32
    #define SCREEN_WIDTH    (EMU_WIDTH*EMU_SCALE)
    #define SCREEN_HEIGHT   (EMU_HEIGHT*EMU_SCALE)
#endif

#define Z_NEAR          0.1f
#define Z_FAR           5000.0f
#define PERSPECTIVE_FOV 45.f
#define ORTHO_SIZE      100.f
#define FPS_CAP         60.f


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

int rasterize(POST_VS_VERTEX* v0, POST_VS_VERTEX* v1, POST_VS_VERTEX* v2);

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
    
#ifdef TEST_MESH
    std::vector<std::array<std::array<int,2>, 3>> triangle_coord = \
        CreateTriangleMesh(EMU_WIDTH, EMU_HEIGHT,TMESH_SUBDIV);
#endif
#ifdef TEST_DX10
    std::vector<std::array<std::array<float,2>, 3>> triangle_coord;
    triangle_coord.push_back({{{{0.5,0.5}},{{1.5,3.5}},{{5.5,1.5}}}});
#endif

    std::vector<std::array<std::array<fpm::fixed_24_8,2>, 3>>   triangles_fixed;
    std::vector<uint32_t>                           triangle_color;

    for (auto& t: triangle_coord){
        // Converting float triangle coordinates to fixed point
        // auto t1 = t;
        std::random_shuffle(t.begin(), t.end()); // The rasterizer has to work for all input arrangements
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

        // print_triangle(t);
        if(rasterize(&p0, &p1, &p2)==0) {
            std::cout << "Failed" << std::endl;
        }
        std::cout << std::flush;
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


int rasterize(POST_VS_VERTEX* v0, POST_VS_VERTEX* v1, POST_VS_VERTEX* v2) {
    fpm::fixed_24_8 x0 = v0->screen_position[0];
    fpm::fixed_24_8 y0 = v0->screen_position[1];
    fpm::fixed_24_8 x1 = v1->screen_position[0];
    fpm::fixed_24_8 y1 = v1->screen_position[1];
    fpm::fixed_24_8 x2 = v2->screen_position[0];
    fpm::fixed_24_8 y2 = v2->screen_position[1];
    // Triangle setup
    fpm::fixed_24_8 step0_x, step0_y, step1_x, step1_y, step2_x, step2_y;   // Pixel Steps
    fpm::fixed_24_8 edge0, edge1, edge2;                                    // Edge functions
    int32_t         left_edge, right_edge, upper_edge, lower_edge;          // Pixel bounds
    int32_t         x, y;                                                   // Pixel Raster Coordinates
    // If it's not even an triangle...
    if ((x0 == x1) && (x1 == x2))   return 0;
    if ((y0 == y1) && (y1 == y2))   return 0;

    // Bonding boxes
    left_edge  = (int)floor(std::min({x0, x1, x2}));
    right_edge = (int) ceil(std::max({x0, x1, x2}));
    upper_edge = (int)floor(std::min({y0, y1, y2}));
    lower_edge = (int) ceil(std::max({y0, y1, y2}));
    // std::cout << "L:" << left_edge << " R:" <<  right_edge << " T:" <<  upper_edge << " B:" <<  lower_edge << std::endl << std::flush;
    bool flat_top = ((upper_edge==static_cast<int>(y0)?1:0)+(upper_edge==static_cast<int>(y1)?1:0)+(upper_edge==static_cast<int>(y2)?1:0))==2;
    if(flat_top)
        std::cout << "is flat top" << std::endl << std::flush;
    // Step vectors
    step0_x = y2 - y0;
    step0_y = x0 - x2;
    step1_x = y0 - y1;
    step1_y = x1 - x0;
    step2_x = y1 - y2;
    step2_y = x2 - x1;

    // std::cout << "(" <<step0_x << "," << step0_y << "),(" <<step1_x << "," << step1_y << "),("<<step2_x << "," << step2_y << ")" << std::endl;
    // std::cout << "(" << (float)step0_x/(float)step0_y << "),(" << (float)step1_x/(float)step1_y << "),("<< (float)step2_x/(float)step2_y << ")" << std::endl;

    // This is awful
    // Determining the sign of the edge functions
    // fpm::fixed_24_8 xc = (x0+x1+x2)/3;
    // fpm::fixed_24_8 yc = (y0+y1+y2)/3;
    // bool s0 = static_cast<int>((xc - x0) * step0_x + (yc - y0) * step0_y) < 0;
    // bool s1 = static_cast<int>((xc - x1) * step1_x + (yc - y1) * step1_y) < 0;
    // bool s2 = static_cast<int>((xc - x2) * step2_x + (yc - y2) * step2_y) < 0;

    bool negate = static_cast<int>( x0*y1 - x0*y2 - x1*y0 + x1*y2 + x2*y0 - x2*y1 ) < 0;

#define EVAL_EDGES do {\
    edge0 = (negate?-1:1)*((x - x0) * step0_x + (y - y0) * step0_y); \
    edge1 = (negate?-1:1)*((x - x1) * step1_x + (y - y1) * step1_y); \
    edge2 = (negate?-1:1)*((x - x2) * step2_x + (y - y2) * step2_y); \
} while(0)

#define IMPL0
// TODO: left edge and top edge
int rastcnt = 0;
#ifdef IMPL0
    bool fill_rule;
    bool at_flat_top;
    for (y=upper_edge;y<lower_edge+2;y++){
        at_flat_top = (y==upper_edge&&flat_top);
        fill_rule = true;
        for (x=left_edge;x<right_edge+2;x++){
            EVAL_EDGES;
            int e0 = static_cast<int>(edge0);
            int e1 = static_cast<int>(edge1);
            int e2 = static_cast<int>(edge2);
            bool in_area = (fill_rule||at_flat_top) ? (e0 >= 0) && (e1 >= 0) && (e2 >= 0) : (e0 > 0) && (e1 > 0) && (e2 > 0);
            if (in_area){
                fill_rule = false;
                uint32_t color;
                uint32_t* pix_addr = ((uint32_t*)(semu->pixels))+(y*EMU_WIDTH+x);
                if(*pix_addr != 0x000000FF) {
                    color = 0x0000FFFF;
                } else {
                    color = color_hack;
                }
                *pix_addr = color;
                rastcnt += 1;
            }
        }
    }

    return rastcnt;
#endif
#ifdef IMPL1

    int rastcnt = 0;
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
                rastcnt++;
            }
            if(hysteresis)
                hysteresis = false;
            if(in_area)
                hysteresis = true;
        }
        hysteresis = false;
    }
    return rastcnt;
#endif
}

