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
#include "../vecmath.h"
#include "psst/math/vector.hpp"
#include "fixed.hpp"
#include "fixed_math.hpp"
#include "fixed_ios.hpp"


using s24p8_t     = fpm::fixed_24_8;
using VEC2_24p8_t = psst::math::vector<s24p8_t,2>;
using VEC3_24p8_t = psst::math::vector<s24p8_t,3>;
using VEC4_24p8_t = psst::math::vector<s24p8_t,4>;

// std::mt19937 generator {114514};

#define TEST_DX10
#define RANDOMIZE

#ifdef TEST_MESH
    #define EMU_WIDTH       320
    #define EMU_HEIGHT      240
    #define EMU_SCALE       4
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

s24p8_t fpm0 = static_cast<s24p8_t>(0);

void print_triangle(std::array<std::array<s24p8_t,2>, 3> t) {
    std::cout << "< [" <<  t[0][0] << ',' <<t[0][1];
    std::cout << "] [" <<  t[1][0] << ',' <<t[1][1];
    std::cout << "] [" <<  t[2][0] << ',' <<t[2][1] << "] >" << std::endl;
    return;
};

typedef union {
    // VEC4 position; // Not kept after rasterization step, only for clipping
    s24p8_t screen_position[4];
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
    triangle_coord.push_back({{{{13.0,1.0}},{{14.0,2.0}},{{14.0,4.0}}}});
    triangle_coord.push_back({{{{13.0,7.0}},{{15.0,7.0}},{{15.0,5.0}}}});
    triangle_coord.push_back({{{{11.0,4.0}},{{11.0,6.0}},{{12.0,5.0}}}});
    triangle_coord.push_back({{{{4.5,5.5}},{{6.5,3.5}},{{7.5,6.5}}}});
    triangle_coord.push_back({{{{0.5,0.5}},{{1.5,3.5}},{{5.5,1.5}}}});
    triangle_coord.push_back({{{{4.0,0.0}},{{4.0,0.0}},{{4.0,0.0}}}});
    triangle_coord.push_back({{{{7.25,2.0}},{{11.25,2.0}},{{9.25,0.25}}}});
    triangle_coord.push_back({{{{4.75,0.75}},{{5.75,0.75}},{{5.75,-0.25}}}});
    triangle_coord.push_back({{{{6,1}},{{7,1}},{{7,0}}}});
    triangle_coord.push_back({{{{0.5,5.5}},{{4.5,5.5}},{{6.5,3.5}}}});
    triangle_coord.push_back({{{{6.5,3.5}},{{7.5,6.5}},{{9,5}}}});
    triangle_coord.push_back({{{{7.25,2.0}},{{11.25,2.0}},{{9,4.75}}}});
    triangle_coord.push_back({{{{13.0,7.0}},{{13.0,5.0}},{{15.0,5.0}}}});
    triangle_coord.push_back({{{{9.0,7.0}},{{10.0,7.0}},{{9.0,9.0}}}});
    triangle_coord.push_back({{{{13.0,1.0}},{{14.0,2.0}},{{14.5,-0.5}}}});
#endif

    std::vector<std::array<std::array<s24p8_t,2>, 3>>   triangles_fixed;
    std::vector<uint32_t>                           triangle_color;

    for (auto& t: triangle_coord){
        // Converting float triangle coordinates to fixed point
        // auto t1 = t;
#ifdef RANDOMIZE
        std::random_shuffle(t.begin(), t.end()); // The rasterizer has to work for all input arrangements
#endif
        triangles_fixed.push_back({{
            {{static_cast<s24p8_t>(t[0][0]),static_cast<s24p8_t>(t[0][1])}},
            {{static_cast<s24p8_t>(t[1][0]),static_cast<s24p8_t>(t[1][1])}},
            {{static_cast<s24p8_t>(t[2][0]),static_cast<s24p8_t>(t[2][1])}}
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
        SDL_SoftStretch(semu, &rsrc, sscr, &rdst);
        SDL_Flip(sscr);
        SDL_PollEvent(&event);
        SDL_Delay(10);
    }
#ifdef TEST_DX10
    // Draw grid
    for (int x=0;x<EMU_WIDTH;x++){
        Draw_VLine(sscr, x*EMU_SCALE, 0, SCREEN_HEIGHT,0x7F7F7FFF);
    }
    for (int y=0;y<EMU_HEIGHT;y++){
        Draw_HLine(sscr, 0, y*EMU_SCALE, SCREEN_WIDTH, 0x7F7F7FFF);
    }
    
    for (auto& t: triangles_fixed){
        // Draw triangles
        uint32_t c00 = (EMU_SCALE*(static_cast<float>(t[0][0])+0.5f));
        uint32_t c01 = (EMU_SCALE*(static_cast<float>(t[0][1])+0.5f));
        uint32_t c10 = (EMU_SCALE*(static_cast<float>(t[1][0])+0.5f));
        uint32_t c11 = (EMU_SCALE*(static_cast<float>(t[1][1])+0.5f));
        uint32_t c20 = (EMU_SCALE*(static_cast<float>(t[2][0])+0.5f));
        uint32_t c21 = (EMU_SCALE*(static_cast<float>(t[2][1])+0.5f));
        if((c00==c10)&&(c00==c20)&&(c01==c11)&&(c01==c21)){
            Draw_Line(sscr,c00-1,c01-1,c00+1,c01+1,0x00FF00FF);
            Draw_Line(sscr,c00-1,c01+1,c00+1,c01-1,0x00FF00FF);
        } else {
            Draw_Line(sscr, c00, c01, c10, c11, 0x00FF00FF);
            Draw_Line(sscr, c20, c21, c10, c11, 0x00FF00FF);
            Draw_Line(sscr, c00, c01, c20, c21, 0x00FF00FF);
        }
    }
#endif
    SDL_Flip(sscr);
    SDL_PollEvent(&event);

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
            // SDL_SoftStretch(semu, &rsrc, sscr, &rdst);
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
    s24p8_t x0 = v0->screen_position[0];
    s24p8_t y0 = v0->screen_position[1];
    s24p8_t x1 = v1->screen_position[0];
    s24p8_t y1 = v1->screen_position[1];
    s24p8_t x2 = v2->screen_position[0];
    s24p8_t y2 = v2->screen_position[1];
    // Triangle setup
    // s24p8_t step0_x, step0_y, step1_x, step1_y, step2_x, step2_y;   // Pixel Steps
    s24p8_t w0, w1, w2;                                    // Edge functions
    int32_t left_edge, right_edge, upper_edge, lower_edge;          // Pixel bounds
    int32_t x, y;                                                   // Pixel Raster Coordinates
    bool negate = false;

    // If it's not even an triangle...
    if ((x0 == x1) && (x1 == x2))   return 0;
    if ((y0 == y1) && (y1 == y2))   return 0;
    
    // Culling
    VEC3_24p8_t vtx0 = {x0,y0,fpm0};
    VEC3_24p8_t vtx1 = {x1,y1,fpm0};
    VEC3_24p8_t vtx2 = {x2,y2,fpm0};
    VEC3_24p8_t vec0 = vtx1-vtx0;
    VEC3_24p8_t vec1 = vtx2-vtx1;
    s24p8_t signed_area = vec0[0] * vec1[1] - vec0[1] * vec1[0];
    if(signed_area <= fpm0){
        std::cout<<"Culled!"<<std::endl;
        // negate = true;
        auto t = v1;
        v1 = v2;
        v2 = t;

        x0 = v0->screen_position[0];
        y0 = v0->screen_position[1];
        x1 = v1->screen_position[0];
        y1 = v1->screen_position[1];
        x2 = v2->screen_position[0];
        y2 = v2->screen_position[1];

        vtx0 = {x0,y0,fpm0};
        vtx1 = {x1,y1,fpm0};
        vtx2 = {x2,y2,fpm0};
    }

    // Bonding boxes
    left_edge  = (int)floor(std::min({x0, x1, x2}));
    right_edge = (int) ceil(std::max({x0, x1, x2}));
    upper_edge = (int)floor(std::min({y0, y1, y2}));
    lower_edge = (int) ceil(std::max({y0, y1, y2}));

    VEC3_24p8_t ev0 = vtx2 - vtx0;
    VEC3_24p8_t ev1 = vtx0 - vtx1; 
    VEC3_24p8_t ev2 = vtx1 - vtx2;

#define EDGE_FUNC(a,b,c) \
    ((c[0] - a[0]) * (b[1] - a[1]) - (c[1] - a[1]) * (b[0] - a[0]))
#define EVAL_EDGES(x,y) do {\
    w0 = (negate?-1:1)*EDGE_FUNC(vtx0,vtx2,(VEC3_24p8_t{static_cast<s24p8_t>(x),static_cast<s24p8_t>(y),fpm0})); \
    w1 = (negate?-1:1)*EDGE_FUNC(vtx1,vtx0,(VEC3_24p8_t{static_cast<s24p8_t>(x),static_cast<s24p8_t>(y),fpm0})); \
    w2 = (negate?-1:1)*EDGE_FUNC(vtx2,vtx1,(VEC3_24p8_t{static_cast<s24p8_t>(x),static_cast<s24p8_t>(y),fpm0})); \
} while(0)
    s24p8_t area = -EDGE_FUNC(vtx0, vtx1, vtx2);

#define IMPL_SOFT

int rastcnt = 0;
#define PIXEL(x,y,c) do{\
    uint32_t cholor; \
    uint32_t* pix_addr = ((uint32_t*)(semu->pixels))+(y*EMU_WIDTH+x); \
    if(*pix_addr != 0x000000FF) cholor = 0x0000FFFF; \
    else                        cholor = c; \
    *pix_addr = cholor; \
    rastcnt += 1; \
}while(0)

#define E0_IS_TOP  (ev0[1]==fpm0 && ev0[0]<fpm0)
#define E1_IS_TOP  (ev1[1]==fpm0 && ev1[0]<fpm0)
#define E2_IS_TOP  (ev2[1]==fpm0 && ev2[0]<fpm0)
#define E0_IS_LEFT (ev0[1]>fpm0)
#define E1_IS_LEFT (ev1[1]>fpm0)
#define E2_IS_LEFT (ev2[1]>fpm0)

#ifdef IMPL_SOFT
    for (y=upper_edge;y<lower_edge+1;y++){
        for (x=left_edge;x<right_edge+1;x++){
            EVAL_EDGES(x,y);
            bool in_area_augmented = true;
            in_area_augmented &= (w0==fpm0 ? (E0_IS_TOP||E0_IS_LEFT) : (w0 > fpm0));
            in_area_augmented &= (w1==fpm0 ? (E1_IS_TOP||E1_IS_LEFT) : (w1 > fpm0));
            in_area_augmented &= (w2==fpm0 ? (E2_IS_TOP||E2_IS_LEFT) : (w2 > fpm0));
            
            if (in_area_augmented){
                int r = static_cast<int>(static_cast<s24p8_t>(255.0f)*w0/area);
                int g = static_cast<int>(static_cast<s24p8_t>(255.0f)*w1/area);
                int b = static_cast<int>(static_cast<s24p8_t>(255.0f)*w2/area);
                uint32_t color = b<<24|g<<16|r<<8|0xFF;
                // uint32_t color = color_hack;
                PIXEL(x,y,color);
            }
        }
    }
#endif


#ifdef IMPL_HARD
    // tbd...
#endif
    return rastcnt;
}

