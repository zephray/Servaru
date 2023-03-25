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

// 2-Stage Loosely coupled rasterizer

#include "tmesh.hh"
#include <iostream>
#include <chrono>
#include <algorithm>    // std::random_shuffle
#include <thread>
#include <iomanip>
#include <assert.h>
#include <SDL.h>
#include <SDL/SDL_draw.h>

#include "fixed.hpp"
#include "fixed_math.hpp"
#include "fixed_ios.hpp"
#include "psst/math/vector.hpp"

// #include "s3d_private.h"

using s24p8_t     = fpm::fixed_24_8;
s24p8_t fpm0 = static_cast<s24p8_t>(0);     // Zero
using VEC2_24p8_t = psst::math::vector<s24p8_t,2>;
using VEC3_24p8_t = psst::math::vector<s24p8_t,3>;
using VEC4_24p8_t = psst::math::vector<s24p8_t,3>;

// #include "../vecmath.h"
// #include "s3d_types.hh"

#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_ ## x
#endif

#ifdef __GNUC__
#  define UNUSED_FUNCTION(x) __attribute__((__unused__)) UNUSED_ ## x
#else
#  define UNUSED_FUNCTION(x) UNUSED_ ## x
#endif

// std::mt19937 generator {114514};

// ----- Settings
#define TEST_MESH_SPARSE
// #define RANDOMIZE
// #define IMPL_SOFT
#define IMPL_HARD
// The edge memory is 1/16th the EMU resolution
#define EDGE_SCALE 8
#define EDGE_GROW 16
#define EDGE_SHOW
// -----

#ifdef TEST_MESH
    #define EMU_WIDTH       320
    // #define EMU_HEIGHT      16
    #define EMU_HEIGHT      240
    #define EMU_SCALE       4
    #define TMESH_SUBDIV    40
#endif

#ifdef TEST_MESH_SPARSE
    #define EMU_WIDTH       320
    #define EMU_HEIGHT      240
    #define EMU_SCALE       4
#endif

#ifdef TEST_DX10
    #define EMU_WIDTH       16
    #define EMU_HEIGHT      8
    #define EMU_SCALE       32
#endif

#define SCREEN_WIDTH    (EMU_WIDTH*EMU_SCALE)
#define SCREEN_HEIGHT   (EMU_HEIGHT*EMU_SCALE)
#define Z_NEAR          0.1f
#define Z_FAR           5000.0f
#define PERSPECTIVE_FOV 45.f
#define ORTHO_SIZE      100.f
#define FPS_CAP         60.f

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

int edge_search(POST_VS_VERTEX** triin, POST_VS_VERTEX** triout, bool* tri_valid, bool* edge_memory);
int rasterize  (POST_VS_VERTEX** triin, bool* tri_valid, bool* edge_memory);

SDL_Surface* semu;  // Frame Buffer
SDL_Surface* sscr;  // Screen (scaled)
// SDL_Surface* s_rastbuf;  // Rasterization Buffer (scaled)

bool edge_memory[(EMU_WIDTH*(EMU_HEIGHT+2*EMU_SCALE))/(EMU_SCALE*EMU_SCALE)+1];

SDL_Rect rsrc(0,0,EMU_WIDTH, EMU_HEIGHT);
SDL_Rect rdst(0,0,SCREEN_WIDTH, SCREEN_HEIGHT);

uint32_t color_hack = 0x7F7F7FFF; // Ugh.. this is bad!
int main([[maybe_unused]]int argc, [[maybe_unused]]char** argv) {
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

#ifdef TEST_MESH_SPARSE
    std::vector<std::array<std::array<float,2>, 3>> triangle_coord;
    triangle_coord.push_back({{{{128,30}},{{200,61}},{{30,100}}}});
    // triangle_coord.push_back({{{{128,30}},{{200,61}},{{200,65}}}});

    // triangle_coord.push_back({{{{64,64}},{{128,128}},{{128,129}}}});
    // triangle_coord.push_back({{{{128,64}},{{64,128}},{{65,129}}}});
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
    std::vector<uint32_t>                               triangle_color;

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
        std::cout << "Rasterizing " << i_trig <<std::endl << std::flush;
    
        uint8_t gray = 255*((float)i_trig/(float)n_trig/2.f)+128;
        color_hack = gray<<24|gray<<16|gray<<8|0xFF;
        i_trig+=1;
        POST_VS_VERTEX* p[3];
        POST_VS_VERTEX p0 = {{t[0][0],t[0][1]}};
        POST_VS_VERTEX p1 = {{t[1][0],t[1][1]}}; 
        POST_VS_VERTEX p2 = {{t[2][0],t[2][1]}};
        p[0] = &p0;
        p[1] = &p1;
        p[2] = &p2;
        // print_triangle(t);
        /*
        if(rasterize(&p0, &p1, &p2)==0) {
            std::cout << "Failed" << std::endl;
        }
        */
        POST_VS_VERTEX** triout;
        bool tri_valid;
        edge_search(p, triout, &tri_valid, edge_memory);
        for(int x=0;x<EMU_WIDTH/EDGE_SCALE;x++){
            for(int y=0;y<EMU_HEIGHT/EDGE_SCALE;y++){
                if(edge_memory[y*EMU_WIDTH/EDGE_SCALE+x]){
                    SDL_Rect dst = {x*EDGE_SCALE-(0.5*EDGE_SCALE), y*EDGE_SCALE-(0.5*EDGE_SCALE), EDGE_SCALE, EDGE_SCALE};
                    // SDL_FillRect(semu, &dst, 0xFF00FFFF);
                }
            }
        }

        rasterize(p, &tri_valid, edge_memory);

        std::cout << std::flush;
        SDL_SoftStretch(semu, &rsrc, sscr, &rdst);
        SDL_Flip(sscr);
        SDL_PollEvent(&event);
        SDL_Delay(10);
    }
#ifdef EDGE_SHOW
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

/*
void edge_line(bool* edge_mem, int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    int32_t dx,dy, s1,s2,status,i, Dx,Dy,sub;
    dx = x1 - x0;
    if (dx >= 0) s1 = 1;
    else         s1 = -1;
    dy = y1 - y0;
    if (dy >= 0) s2 = 1;
    else         s2 = -1;
    Dx = abs(x1 - x0);
    Dy = abs(y1 - y0);
    if (Dy > Dx) {
        auto temp = Dx;
        Dx = Dy;
        Dy = temp;
        status = 1;
    }
    else
        status = 0;
    sub = 2 * Dy - Dx;
    for (i = 0; i < Dx; i++) {
        edge_mem[y0*EMU_WIDTH/EDGE_SCALE+x0] = true;
        if (sub >= 0) {
            if(status == 1) x0 += s1;
            else            y0 += s2;
            sub -= 2 * Dx;
        }
        if (status == 1) y0 += s2;
        else             x0 += s1;
        sub += 2 * Dy;
    }
}
*/

// Stage 1 of the rasterization pipeline
// This state machine computes the boundary of each triangle
int edge_search(POST_VS_VERTEX** triin, POST_VS_VERTEX** triout, bool* tri_valid, bool* edge_memory){
    *tri_valid = true;
    s24p8_t vx[3], vy[3];
    for(int i=0;i<3;i++){
        vx[i] = triin[i]->screen_position[0]/EDGE_SCALE;
        vy[i] = triin[i]->screen_position[1]/EDGE_SCALE;
        // memcpy(triout[i],triin[i],sizeof(triin[i]));
    }

    // Triangle setup
    // s24p8_t step0_x, step0_y, step1_x, step1_y, step2_x, step2_y;   // Pixel Steps
    s24p8_t w0, w1, w2;                                 // Edge functions
    [[maybe_unused]] int32_t left_edge,  right_edge; 
    [[maybe_unused]] int32_t upper_edge, lower_edge;    // Pixel bounds
    int32_t x, y;                                       // Pixel Raster Coordinates
    // If it's not even an triangle...
    if (((vx[0] == vx[1]) && (vx[1] == vx[2]))||((vy[0] == vy[1]) && (vy[1] == vy[2]))){
        *tri_valid = false;
        return -1;
    }   
    
    // Culling
    VEC3_24p8_t vtx0 = {vx[0],vy[0],fpm0};
    VEC3_24p8_t vtx1 = {vx[1],vy[1],fpm0};
    VEC3_24p8_t vtx2 = {vx[2],vy[2],fpm0};
    VEC3_24p8_t vec0 = vtx1-vtx0;
    VEC3_24p8_t vec1 = vtx2-vtx1;
    s24p8_t signed_area = vec0[0] * vec1[1] - vec0[1] * vec1[0];
    if(signed_area <= fpm0){
        // if(s3d_context.face_culling){
        if(true){
            std::cout << "Culled!" <<std::endl <<std::flush;
            *tri_valid = false;
            return -1;
        } else {
            auto t = triin[1];
            triin[1] = triin[2];
            triin[2] = t;
            for(int i=0;i<3;i++){
                vx[i] = triin[i]->screen_position[0]/EDGE_SCALE;
                vy[i] = triin[i]->screen_position[1]/EDGE_SCALE;
                // memcpy(triout[i],triin[i],sizeof(triin[i]));
            }
            vtx0 = {vx[0],vy[0],fpm0};
            vtx1 = {vx[1],vy[1],fpm0};
            vtx2 = {vx[2],vy[2],fpm0};
        }
    }
    // Bonding boxes
    left_edge  = (int)floor(std::min({vx[0], vx[1], vx[2]}));
    right_edge = (int) ceil(std::max({vx[0], vx[1], vx[2]})+1);
    upper_edge = (int)floor(std::min({vy[0], vy[1], vy[2]}));
    lower_edge = (int) ceil(std::max({vy[0], vy[1], vy[2]})+1);
    if(left_edge<0)                      left_edge  = 0;
    if(right_edge>EMU_WIDTH/EDGE_SCALE)  right_edge = EMU_WIDTH/EDGE_SCALE;
    if(upper_edge<0)                     upper_edge = 0;
    if(lower_edge>EMU_HEIGHT/EDGE_SCALE+2) lower_edge = EMU_HEIGHT/EDGE_SCALE+2;

    VEC3_24p8_t ev0 = vtx2 - vtx0;
    VEC3_24p8_t ev1 = vtx0 - vtx1; 
    VEC3_24p8_t ev2 = vtx1 - vtx2;

#define EDGE_FUNC(a,b,c) \
    ((c[0] - a[0]) * (b[1] - a[1]) - (c[1] - a[1]) * (b[0] - a[0]))
#define EVAL_EDGES(x,y) do {\
    w0 = EDGE_FUNC(vtx0,vtx2,(VEC3_24p8_t{static_cast<s24p8_t>(x),static_cast<s24p8_t>(y),fpm0})); \
    w1 = EDGE_FUNC(vtx1,vtx0,(VEC3_24p8_t{static_cast<s24p8_t>(x),static_cast<s24p8_t>(y),fpm0})); \
    w2 = EDGE_FUNC(vtx2,vtx1,(VEC3_24p8_t{static_cast<s24p8_t>(x),static_cast<s24p8_t>(y),fpm0})); \
} while(0)

    int rastcnt = 0;
#ifdef IMPL_SOFT
    for (y = upper_edge;y<lower_edge;y++){
        for (x = left_edge;x<right_edge;x++){
            bool mask=true;
            EVAL_EDGES(x,y);
            mask &= (w0 > fpm0 - static_cast<s24p8_t>(EDGE_GROW));
            mask &= (w1 > fpm0 - static_cast<s24p8_t>(EDGE_GROW));
            mask &= (w2 > fpm0 - static_cast<s24p8_t>(EDGE_GROW));
            if(mask) edge_memory[y*EMU_WIDTH/EDGE_SCALE+x] = true;
        }
    }
#endif /*IMPL_SOFT*/

#ifdef IMPL_HARD
    x = left_edge;
    y = upper_edge;
    w0 = EDGE_FUNC(vtx0, vtx2, (VEC3_24p8_t{static_cast<s24p8_t>(x),static_cast<s24p8_t>(y),fpm0}));
    w1 = EDGE_FUNC(vtx1, vtx0, (VEC3_24p8_t{static_cast<s24p8_t>(x),static_cast<s24p8_t>(y),fpm0}));
    w2 = EDGE_FUNC(vtx2, vtx1, (VEC3_24p8_t{static_cast<s24p8_t>(x),static_cast<s24p8_t>(y),fpm0}));
    bool r1_active = true;
    typedef enum{
        SS_RIGHT,
        SS_LEFT
    } R1_STATE;
    const int ITER_LIMIT = EMU_WIDTH*EMU_HEIGHT;
    int iter_cnt = 0;
    R1_STATE ss = SS_RIGHT;
    while(r1_active){
        bool inside = true;
        inside &= (w0 > fpm0 - static_cast<s24p8_t>(EDGE_GROW));
        inside &= (w1 > fpm0 - static_cast<s24p8_t>(EDGE_GROW));
        inside &= (w2 > fpm0 - static_cast<s24p8_t>(EDGE_GROW));
        if(inside){
            edge_memory[y*EMU_WIDTH/EDGE_SCALE+x] = true;
            rastcnt++;
        }
        switch(ss){
            case SS_RIGHT:
                if(x==right_edge){
                    y++; 
                    w0 -= (vtx2[0] - vtx0[0]);
                    w1 -= (vtx0[0] - vtx1[0]);
                    w2 -= (vtx1[0] - vtx2[0]);
                    ss=SS_LEFT;
                } else {
                    x++;
                    w0 += (vtx2[1] - vtx0[1]);
                    w1 += (vtx0[1] - vtx1[1]);
                    w2 += (vtx1[1] - vtx2[1]);
                }
                break;
            case SS_LEFT:
                if(x==left_edge){
                    y++; 
                    w0 -= (vtx2[0] - vtx0[0]);
                    w1 -= (vtx0[0] - vtx1[0]);
                    w2 -= (vtx1[0] - vtx2[0]);
                    ss=SS_RIGHT;
                } else {
                    x--;
                    w0 -= (vtx2[1] - vtx0[1]);
                    w1 -= (vtx0[1] - vtx1[1]);
                    w2 -= (vtx1[1] - vtx2[1]);
                }
                break;
        }
        if(y>=lower_edge)
            r1_active = false;
        if(iter_cnt++>ITER_LIMIT){
            std::cout << "Infinite Loop" << std::endl << std::flush;
            r1_active = false;
        }
    }
#endif /*IMPL_HARD*/
    return rastcnt;
}

// stage 2 of the rasterization pipeline
// This state machine tests and fills all the pixels within the boundary
int rasterize(POST_VS_VERTEX** triin, bool* tri_valid, bool* edge_memory){
    if(!*tri_valid)
        return -1;
    s24p8_t vx[3], vy[3];
    for(int i=0;i<3;i++){
        vx[i] = triin[i]->screen_position[0];
        vy[i] = triin[i]->screen_position[1];
    }
    
    // Triangle setup
    // s24p8_t step0_x, step0_y, step1_x, step1_y, step2_x, step2_y;   // Pixel Steps
    s24p8_t w0, w1, w2;                                 // Edge functions
    [[maybe_unused]] int32_t left_edge,  right_edge; 
    [[maybe_unused]] int32_t upper_edge, lower_edge;    // Pixel bounds
    int32_t x, y;                                       // Pixel Raster Coordinates
    bool negate = false;
    
    // Culling
    VEC3_24p8_t vtx0 = {vx[0],vy[0],fpm0};
    VEC3_24p8_t vtx1 = {vx[1],vy[1],fpm0};
    VEC3_24p8_t vtx2 = {vx[2],vy[2],fpm0};
    VEC3_24p8_t vec0 = vtx1-vtx0;
    VEC3_24p8_t vec1 = vtx2-vtx1;
    s24p8_t signed_area = vec0[0] * vec1[1] - vec0[1] * vec1[0];
    if(signed_area <= fpm0){
        std::cout<<"ERROR: Should have been Culled!"<<std::endl;
        return -1;
    }

    // Bonding boxes
    left_edge  = (int)floor(std::min({vx[0], vx[1], vx[2]}));
    right_edge = (int) ceil(std::max({vx[0], vx[1], vx[2]}));
    upper_edge = (int)floor(std::min({vy[0], vy[1], vy[2]}));
    lower_edge = (int) ceil(std::max({vy[0], vy[1], vy[2]}));

    VEC3_24p8_t ev0 = vtx2 - vtx0;
    VEC3_24p8_t ev1 = vtx0 - vtx1; 
    VEC3_24p8_t ev2 = vtx1 - vtx2;

#define EDGE_FUNC(a,b,c) \
    ((c[0] - a[0]) * (b[1] - a[1]) - (c[1] - a[1]) * (b[0] - a[0]))
#define EVAL_EDGES(x,y) do {\
    w0 = EDGE_FUNC(vtx0,vtx2,(VEC3_24p8_t{static_cast<s24p8_t>(x),static_cast<s24p8_t>(y),fpm0})); \
    w1 = EDGE_FUNC(vtx1,vtx0,(VEC3_24p8_t{static_cast<s24p8_t>(x),static_cast<s24p8_t>(y),fpm0})); \
    w2 = EDGE_FUNC(vtx2,vtx1,(VEC3_24p8_t{static_cast<s24p8_t>(x),static_cast<s24p8_t>(y),fpm0})); \
} while(0)
    s24p8_t area = -EDGE_FUNC(vtx0, vtx1, vtx2);

int rastcnt = 0;
#define PIXEL(x,y,c) do{\
    uint32_t __color; \
    uint32_t* pix_addr = ((uint32_t*)(semu->pixels))+((y)*EMU_WIDTH+(x)); \
    if(*pix_addr != 0x000000FF) __color = 0x0000FFFF; \
    else                        __color = c; \
    *pix_addr = __color; \
    rastcnt += 1; \
}while(0)

#define E0_IS_TOP  (ev0[1]==fpm0 && ev0[0]<fpm0)
#define E1_IS_TOP  (ev1[1]==fpm0 && ev1[0]<fpm0)
#define E2_IS_TOP  (ev2[1]==fpm0 && ev2[0]<fpm0)
#define E0_IS_LEFT (ev0[1]>fpm0)
#define E1_IS_LEFT (ev1[1]>fpm0)
#define E2_IS_LEFT (ev2[1]>fpm0)

    // Quad
    s24p8_t w0_quad[4];
    s24p8_t w1_quad[4];
    s24p8_t w2_quad[4];
    const int dx[] = {0,1,0,1};
    const int dy[] = {0,0,1,1};
#ifdef IMPL_SOFT
    for (y = upper_edge;y<lower_edge;y+=2){
        for (x = left_edge;x<right_edge;x+=2){
            bool mask[4] = {true, true, true, true};
            for(int k=0;k<4;k++){
                EVAL_EDGES(x+dx[k],y+dy[k]);
                w0_quad[k] = w0;
                w1_quad[k] = w1;
                w2_quad[k] = w2;
                mask[k] &= (w0_quad[k]==fpm0 ? (E0_IS_TOP||E0_IS_LEFT) : (w0_quad[k] > fpm0));
                mask[k] &= (w1_quad[k]==fpm0 ? (E1_IS_TOP||E1_IS_LEFT) : (w1_quad[k] > fpm0));
                mask[k] &= (w2_quad[k]==fpm0 ? (E2_IS_TOP||E2_IS_LEFT) : (w2_quad[k] > fpm0));
            }
            // Essentially the fragment shader
            if(edge_memory[((y+EDGE_SCALE/2)/EDGE_SCALE)*(EMU_WIDTH/EDGE_SCALE)+((x+EDGE_SCALE/2)/EDGE_SCALE)]){
            // if(true){
                for(int k=0;k<4;k++) {
                    if (mask[k]){
                        int r = static_cast<int>(static_cast<s24p8_t>(255.0f)*w0_quad[k]/area);
                        int g = static_cast<int>(static_cast<s24p8_t>(255.0f)*w1_quad[k]/area);
                        int b = static_cast<int>(static_cast<s24p8_t>(255.0f)*w2_quad[k]/area);
                        PIXEL(x+dx[k],y+dy[k],b<<24|g<<16|r<<8|0xFF);
                    }
                }
            } else {
                // for(int k=0;k<4;k++)
                //     PIXEL(x+dx[k],y+dy[k],0x00FF00FF);
            
            }
        }
    }
#endif

#ifdef IMPL_HARD
    // Grid Coordinates (x 1/EDGE_SCALE)
    uint32_t x_grid = left_edge/EDGE_SCALE;  
    uint32_t y_grid = upper_edge/EDGE_SCALE;
    
    // Step Vectors
    s24p8_t s0x =  (vtx2[1] - vtx0[1]);
    s24p8_t s0y = -(vtx2[0] - vtx0[0]);
    s24p8_t s1x =  (vtx0[1] - vtx1[1]);
    s24p8_t s1y = -(vtx0[0] - vtx1[0]);
    s24p8_t s2x =  (vtx1[1] - vtx2[1]);
    s24p8_t s2y = -(vtx1[0] - vtx2[0]);
    // First edge function computation (belongs to the SETUP stage)
    int32_t x0 = x_grid*EDGE_SCALE-(0.5*EDGE_SCALE);
    int32_t y0 = y_grid*EDGE_SCALE-(0.5*EDGE_SCALE);
    x = x0; y = y0;
    s24p8_t w00 = EDGE_FUNC(vtx0, vtx2, (VEC3_24p8_t{static_cast<s24p8_t>(x),static_cast<s24p8_t>(y),fpm0}));
    s24p8_t w10 = EDGE_FUNC(vtx1, vtx0, (VEC3_24p8_t{static_cast<s24p8_t>(x),static_cast<s24p8_t>(y),fpm0}));
    s24p8_t w20 = EDGE_FUNC(vtx2, vtx1, (VEC3_24p8_t{static_cast<s24p8_t>(x),static_cast<s24p8_t>(y),fpm0}));
    w0  = w00;
    w1  = w10;
    w2  = w20;


    // Grid Walker FSM
    typedef enum {
        SS_SEARCH_LEFTEDGE_OUTSIDE,     
        SS_SEARCH_RIGHTEDGE_OUTSIDE,
        SS_SEARCH_LEFTEDGE_INSIDE,
        SS_SEARCH_RIGHTEDGE_INSIDE,
        SS_SWEEP_RIGHT,
        SS_SWEEP_LEFT,
        SS_DOWN_RIGHTEDGE,
        SS_DOWN_LEFTEDGE
    } R2_GRID_STATE;
    typedef enum {
        DIR_LEFT,
        DIR_RIGHT,
        DIR_DOWN,
        DIR_LASTX_P1,
        DIR_LASTX_M1,
        DIR_INVALID
    } R2_GRID_DIR;

    typedef enum {
        SS_BLK_SWEEP_RIGHT,
        SS_BLK_SWEEP_LEFT
    } R2_BLOCK_STATE;

    typedef enum {
        DIR_BLK_LEFT,
        DIR_BLK_RIGHT,
        DIR_BLK_DOWN,
        DIR_BLK_INVALID
    } R2_BLOCK_DIR;

    R2_GRID_STATE ss_gw      = SS_SEARCH_LEFTEDGE_OUTSIDE;
    R2_GRID_DIR   ss_gw_dir  = DIR_INVALID;
    const int GW_LOOP_LIMIT = (EMU_HEIGHT * EMU_WIDTH) / (EDGE_SCALE*EDGE_SCALE);
    int       gw_loop_cnt  = 0;
    uint32_t  gw_lastx     = 0;
    uint32_t  gw_lastx_raw = x;
    s24p8_t   gw_last_w0;
    s24p8_t   gw_last_w1;
    s24p8_t   gw_last_w2;

    bool grid_walker_active = true;
    #define OUTSIDE (edge_memory[y_grid*EMU_WIDTH/EDGE_SCALE+x_grid]==false)
    while(grid_walker_active) {
        if(!OUTSIDE){
            // Block Rasterizer FSM
            s24p8_t w0_local = w0;
            s24p8_t w1_local = w1;
            s24p8_t w2_local = w2;
            uint32_t x_local = x;
            uint32_t y_local = y;
            for(uint32_t y_var=0;y_var<EDGE_SCALE;y_var+=2){
                for (uint32_t x_var=0;x_var<EDGE_SCALE;x_var+=2){
                    bool mask[4] = {true, true, true, true};
                    for(int k=0;k<4;k++){
                        // TODO: Turn this into an incremental FSM
                        w0_quad[k] = w00+(x_local-x0+x_var+dx[k])*s0x+(y_local-y0+y_var+dy[k])*s0y;
                        w1_quad[k] = w10+(x_local-x0+x_var+dx[k])*s1x+(y_local-y0+y_var+dy[k])*s1y;
                        w2_quad[k] = w20+(x_local-x0+x_var+dx[k])*s2x+(y_local-y0+y_var+dy[k])*s2y;
                        mask[k] &= (w0_quad[k]==fpm0 ? (E0_IS_TOP||E0_IS_LEFT) : (w0_quad[k] > fpm0));
                        mask[k] &= (w1_quad[k]==fpm0 ? (E1_IS_TOP||E1_IS_LEFT) : (w1_quad[k] > fpm0));
                        mask[k] &= (w2_quad[k]==fpm0 ? (E2_IS_TOP||E2_IS_LEFT) : (w2_quad[k] > fpm0));
                    }
                    // Essentially the fragment shader
                    if((mask[0]|mask[1]|mask[2]|mask[3])!=false){
                        std::cout << "Rasterize Block: " << x_local << ',' << y_local << std::endl;
                        for(int k=0;k<4;k++) {
                            if (mask[k]){
                                int r = static_cast<int>(static_cast<s24p8_t>(255.0f)*w0_quad[k]/area);
                                int g = static_cast<int>(static_cast<s24p8_t>(255.0f)*w1_quad[k]/area);
                                int b = static_cast<int>(static_cast<s24p8_t>(255.0f)*w2_quad[k]/area);
                                PIXEL(x_local+x_var+dx[k],y_local+y_var+dy[k],b<<24|g<<16|r<<8|0xFF);
                                rastcnt++;
                            }
                        }
                    }
                }
            }
            // Draw boxes for now
            // SDL_Rect dst = {x, y, EDGE_SCALE, EDGE_SCALE};
            // SDL_FillRect(semu, &dst, 0xFF00FFFF);
        }
        if(gw_loop_cnt++>GW_LOOP_LIMIT){
            // Grid Walker Loop Limiter
            std::cout << "Endless loop" << std::endl;
            return -1;
        }
        // Terminating Condition
        if(y_grid>=lower_edge/EDGE_SCALE+2){
            std::cout << "Rast Done" << std::endl;
            grid_walker_active = false;
        }

        switch(ss_gw) {
        case SS_SEARCH_LEFTEDGE_OUTSIDE:
            if(x_grid>=EMU_WIDTH/EDGE_SCALE){
                std::cout<<"Row Missed! SS_SEARCH_LEFTEDGE_OUTSIDE" <<std::endl;
                ss_gw_dir = DIR_DOWN;
                ss_gw = SS_SEARCH_RIGHTEDGE_OUTSIDE;
            } else if(OUTSIDE){
                ss_gw_dir = DIR_RIGHT;
                ss_gw = SS_SEARCH_LEFTEDGE_OUTSIDE;
            } else {
                ss_gw_dir = DIR_RIGHT;
                ss_gw = SS_SWEEP_RIGHT;
            }
            break;
        case SS_SEARCH_RIGHTEDGE_OUTSIDE:
            if(x_grid<=0){
                std::cout<<"Row Missed! SS_SEARCH_RIGHTEDGE_OUTSIDE" <<std::endl;
                ss_gw_dir = DIR_DOWN;
                ss_gw = SS_SEARCH_LEFTEDGE_OUTSIDE;
            } else if(OUTSIDE){
                ss_gw_dir = DIR_LEFT;
                ss_gw = SS_SEARCH_RIGHTEDGE_OUTSIDE;
            } else {
                ss_gw_dir = DIR_LEFT;
                ss_gw = SS_SWEEP_LEFT;
            }
            break;
        case SS_SEARCH_RIGHTEDGE_INSIDE:
            if(OUTSIDE||(x_grid>=EMU_WIDTH/EDGE_SCALE)){
                ss_gw_dir = DIR_LASTX_M1;
                ss_gw = SS_SWEEP_LEFT;
            } else {
                ss_gw_dir = DIR_RIGHT;
                ss_gw = SS_SEARCH_RIGHTEDGE_INSIDE;
            }
            break;
        case SS_SEARCH_LEFTEDGE_INSIDE:
            if(OUTSIDE||(x_grid<=0)){
                ss_gw_dir = DIR_LASTX_P1;
                ss_gw = SS_SWEEP_RIGHT;
            } else {
                ss_gw_dir = DIR_LEFT;
                ss_gw = SS_SEARCH_LEFTEDGE_INSIDE;
            }
            break;
        case SS_SWEEP_RIGHT:
            if(OUTSIDE||(x_grid>=EMU_WIDTH/EDGE_SCALE)){
                ss_gw_dir = DIR_DOWN;
                ss_gw = SS_DOWN_RIGHTEDGE;
            } else {
                ss_gw_dir = DIR_RIGHT;
                ss_gw = SS_SWEEP_RIGHT;
            }
            break;
        case SS_SWEEP_LEFT:
            if(OUTSIDE||(x_grid<=0)){
                ss_gw_dir = DIR_DOWN;
                ss_gw = SS_DOWN_LEFTEDGE;
            } else {
                ss_gw_dir = DIR_LEFT;
                ss_gw = SS_SWEEP_LEFT;
            }
            break;
        case SS_DOWN_RIGHTEDGE:
            if(OUTSIDE){
                ss_gw_dir = DIR_RIGHT;
                ss_gw = SS_SEARCH_RIGHTEDGE_OUTSIDE;
            } else {
                gw_lastx = x_grid;
                gw_lastx_raw = x;
                ss_gw_dir = DIR_RIGHT;
                ss_gw = SS_SEARCH_RIGHTEDGE_INSIDE;
            }
            break;
        case SS_DOWN_LEFTEDGE:
            if(OUTSIDE){
                ss_gw_dir = DIR_LEFT;
                ss_gw = SS_SEARCH_LEFTEDGE_OUTSIDE;
            } else {
                // lastx = x+2;
                gw_lastx = x_grid;
                gw_lastx_raw = x;
                ss_gw_dir = DIR_LEFT;
                ss_gw = SS_SEARCH_LEFTEDGE_INSIDE;
            }
            break;
        default:
            assert(0);
            return -1;
        } 
        // Walk in the direction
        switch(ss_gw_dir){
            case DIR_LEFT:     x_grid-=1;         w0-=s0x*EDGE_SCALE;           w1-=s1x*EDGE_SCALE;           w2-=s2x*EDGE_SCALE;           x-=EDGE_SCALE; break;
            case DIR_RIGHT:    x_grid+=1;         w0+=s0x*EDGE_SCALE;           w1+=s1x*EDGE_SCALE;           w2+=s2x*EDGE_SCALE;           x+=EDGE_SCALE; break;
            case DIR_DOWN:     y_grid+=1;         w0+=s0y*EDGE_SCALE;           w1+=s1y*EDGE_SCALE;           w2+=s2y*EDGE_SCALE;           y+=EDGE_SCALE; break;
            case DIR_LASTX_P1: x_grid=gw_lastx+1; w0=gw_last_w0+s0x*EDGE_SCALE; w1=gw_last_w1+s1x*EDGE_SCALE; w2=gw_last_w2+s2x*EDGE_SCALE; x=gw_lastx_raw+EDGE_SCALE; break;
            case DIR_LASTX_M1: x_grid=gw_lastx-1; w0=gw_last_w0-s0x*EDGE_SCALE; w1=gw_last_w1-s1x*EDGE_SCALE; w2=gw_last_w2-s2x*EDGE_SCALE; x=gw_lastx_raw-EDGE_SCALE; break;
            case DIR_INVALID: 
            default:          assert(0); return -1;
        }
    } /*while(grid_walker_active)*/
#endif
    return rastcnt;
}

