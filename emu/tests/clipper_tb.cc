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
#include "colormap.hh"
#include "../defs.h"
#include "tmesh.hh"
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <assert.h>
#include <SDL.h>
#include <SDL/SDL_draw.h>
#include "vecmath.h"

#define TMESH_SUBDIV 20
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

uint8_t pbuf[EMU_WIDTH*EMU_HEIGHT];


void my_draw_line(uint8_t* buf,int w, int h, int x1, int y1, int x2, int y2) {
    if((x1<0)||(x1>EMU_WIDTH) ||(x2<0)||(x2>EMU_WIDTH)||\
       (y1<0)||(y1>EMU_HEIGHT)||(y2<0)||(y2>EMU_HEIGHT)){
        std::cout << "Out of bound! ("<<x1<<','<<y1<<") ("<<x2<<','<<y2<<')'<<std::endl;
        return;
    }
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    int cnt = 0;
    while (true) {
        int index = y1*w+x1;
        buf[index] += 1;
        cnt++;
        if(cnt>w*h)
            break;
        if (x1 == x2 && y1 == y2)
            break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
    
}

void apply_cmap(SDL_Surface* s, uint8_t* buf, float K){
    int w = s->w;
    int h = s->h;
    for (int i=0;i<w*h;i++){
        auto v = turbo_srgb_bytes[(int)(round(K*buf[i]))];
        uint8_t r = v[0];
        uint8_t g = v[1];
        uint8_t b = v[2];
        ((uint32_t*)(s->pixels))[i] = b<<24|g<<16|r<<8|0xFF;
    }
}

SDL_Surface* semu;  // Frame Buffer
SDL_Surface* sscr;  // Screen (scaled)
SDL_Rect rsrc(0,0,EMU_WIDTH, EMU_HEIGHT);
SDL_Rect rdst(0,0,SCREEN_WIDTH, SCREEN_HEIGHT);
uint32_t color_hack = 0x7F7F7FFF; // Ugh.. this is bad!


typedef struct {
    POST_VS_VERTEX v[3];
} triangle_t;

std::vector<triangle_t> triangles_1;

void rescale_triangles(std::vector<std::array<std::array<int,2>, 3>> triangles, float K){
    auto xhalf = EMU_WIDTH/2;
    auto yhalf = EMU_HEIGHT/2;
    triangles_1.clear();
    for (auto& t: triangles) {
        POST_VS_VERTEX p0 = {
            {K*(t[0][0]-xhalf)+xhalf,K*(t[0][1]-yhalf)+yhalf}
        };
        POST_VS_VERTEX p1 = {
            {K*(t[1][0]-xhalf)+xhalf,K*(t[1][1]-yhalf)+yhalf}
        };
        POST_VS_VERTEX p2 = {
            {K*(t[2][0]-xhalf)+xhalf,K*(t[2][1]-yhalf)+yhalf}
        };
        triangles_1.push_back(triangle_t({p0,p1,p2}));
    }
}

int main(int argc, char** argv){
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
    float time_delta = 0.0f;
    int last_ticks = SDL_GetTicks();

    int exit = 0;

    float K = 0.1;
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
            K += 0.01;
            rescale_triangles(triangles,K);
            memset(pbuf,0,sizeof(pbuf));
            for (auto& t: triangles_1) {
                auto t00 = t.v[0].screen_position[0];
                auto t01 = t.v[0].screen_position[1];
                auto t10 = t.v[1].screen_position[0];
                auto t11 = t.v[1].screen_position[1];
                auto t20 = t.v[2].screen_position[0];
                auto t21 = t.v[2].screen_position[1];
                my_draw_line(pbuf,EMU_WIDTH,EMU_HEIGHT,t00,t01,t10,t11);
                my_draw_line(pbuf,EMU_WIDTH,EMU_HEIGHT,t20,t21,t10,t11);
                my_draw_line(pbuf,EMU_WIDTH,EMU_HEIGHT,t00,t01,t20,t21);
                // SDL_Delay(100);
            }
            apply_cmap(semu,pbuf,40);
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