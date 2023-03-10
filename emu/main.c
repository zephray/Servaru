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
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include <SDL.h>
#include <math.h>
#include "engine.h"
#include "defs.h"

#define TEST_CUBE
//#define TEST_SPONZA

static void accelerate(float *speed, float *target) {
    if (fabsf(*speed - *target) > 0.01f) {
        *speed += (*target - *speed) / 8.0f;
    }
    else
        *speed = *target;
}

int main(int argc, char *argv[]) {
    SDL_Surface *screen;
    int screen_width  = SCREEN_WIDTH;
    int screen_height = SCREEN_HEIGHT;

    // Initialize the window
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        exit(1);
    }

    SDL_WM_SetCaption("S3D Window", NULL);

    screen = SDL_SetVideoMode(screen_width, screen_height, 32, SDL_SWSURFACE);
    assert(screen);

    SDL_WM_GrabInput(SDL_GRAB_ON);
    SDL_WarpMouse(screen_width/2, screen_height/2);

    s3d_init(screen_width, screen_height);
    printf("Window created\n");

    float aspect = (float)screen_width / (float)screen_height;

    CAMERA camera;
    camera_init(&camera);
    camera_set_projection(&camera, RADIAN(45.0f), aspect, Z_NEAR, Z_FAR);
    camera.position.x = 1;
    camera.position.y = 0;
    camera.position.z = 2;
    camera.yaw = 240.f;
    /*camera.pitch = 0.f;
    camera.position.x = -0.88402f;
    camera.position.y = 0.30422f;
    camera.position.z = 0.13964f;
    camera.yaw = 356.4;
    camera.pitch = -63.8f;*/
    camera_update(&camera);

    SHADER simple_shader;
    shader_init(&simple_shader, "resources/shaders/simple.vs",
            "resources/shaders/simple.fs");

    OBJ *obj;

#ifdef TEST_SPONZA
    obj = mesh_load_obj("resources/crytek_sponza/", "sponza.obj", 1.0f);
#endif
    //obj = mesh_load_obj("resources/sponza/", "sponza.obj", 100.0f);
#ifdef TEST_CUBE
    obj = mesh_load_obj("resources/", "cube.obj", 1.0f);
#endif
    size_t mesh_total_size = 0;
    size_t mesh_total_tri = 0;
    for (size_t i = 0; i < obj->num_meshes; i++) {
        mesh_init(&obj->meshes[i]);
        mesh_total_size += mesh_size(&obj->meshes[i]);
        mesh_total_tri += obj->meshes[i].num_indices / 3;
        //mesh_dump(&obj->meshes[i]);
    }
    printf("Imported %zu meshes.\n", obj->num_meshes);
    printf("Total number of triangles: %zu\n", mesh_total_tri);
    printf("Total size for meshes: %zu KB\n", mesh_total_size / 1024);

    obj->forward_shader = &simple_shader;

    s3d_depth_test(true);
    s3d_face_culling(true);

    bool exit = false;
    float x_velocity = 0.0f;
    float x_target = 0.0f;
    float y_velocity = 0.0f;
    float y_target = 0.0f;
    float z_velocity = 0.0f;
    float z_target = 0.0f;
#ifdef TEST_SPONZA
    const float speed = 8.0f;
#endif
#ifdef TEST_CUBE
    const float speed = 0.2f;
#endif
    int projection = 0; // 0 - perspective, 1 - ortho
    float perspective_fov = PERSPECTIVE_FOV;
    float ortho_size = ORTHO_SIZE;

    // Simple pipeline
    mesh_render_obj(obj, &camera, FORWARD_PASS);
    s3d_render_copy(screen->pixels);
	SDL_Flip(screen);

    float time_delta = 0.0f;
    int last_ticks = SDL_GetTicks();

    //exit = 1;

    while (!exit) {
        int cur_ticks = SDL_GetTicks();
        time_delta -= cur_ticks - last_ticks; // Actual ticks passed since last iteration
        time_delta += 1000.0f / 30.f; // Time allocated for this iteration
        last_ticks = cur_ticks;

        // S3D internally uses SDL for window creation, directly use SDL functions for events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                exit = true;
                break;
            case SDL_MOUSEMOTION:
                camera_rotate(&camera, event.motion.xrel / 4.f, -event.motion.yrel / 4.f);
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    exit = true;
                    break;
                case SDLK_w:
                    x_target = speed;
                    break;
                case SDLK_s:
                    x_target = -speed;
                    break;
                case SDLK_a:
                    y_target = -speed;
                    break;
                case SDLK_d:
                    y_target = speed;
                    break;
                case SDLK_SPACE:
                    z_target = speed;
                    break;
                case SDLK_z:
                    z_target = -speed;
                    break;
                case SDLK_o:
                    camera_set_ortho_projection(&camera, ortho_size, aspect, Z_NEAR, Z_FAR);
                    projection = 1;
                    break;
                case SDLK_p:
                    camera_set_projection(&camera, RADIAN(perspective_fov), aspect, Z_NEAR, Z_FAR);
                    projection = 0;
                    break;
                case SDLK_l:
                    camera_toggle_lookat(&camera);
                    camera_update(&camera);
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
                case SDLK_w:
                    x_target = 0.0f;
                    break;
                case SDLK_s:
                    x_target = 0.0f;
                    break;
                case SDLK_a:
                    y_target = 0.0f;
                    break;
                case SDLK_d:
                    y_target = 0.0f;
                    break;
                case SDLK_SPACE:
                    z_target = 0.0f;
                    break;
                case SDLK_z:
                    z_target = 0.0f;
                    break;
                default:
                    break;
                }
                break;
            }
        }

        accelerate(&x_velocity, &x_target);
        accelerate(&y_velocity, &y_target);
        accelerate(&z_velocity, &z_target);
        if (x_velocity != 0.0f) {
            camera_move(&camera, M_FORWARD, x_velocity);
        }
        if (y_velocity != 0.0f) {
            camera_move(&camera, M_RIGHT, y_velocity);
        }
        if (z_velocity != 0.0f) {
            camera_move(&camera, M_UP, z_velocity);
        }

        // Simple pipeline
        if (time_delta > 0) {
            s3d_clear_color();
            s3d_clear_depth();
            mesh_render_obj(obj, &camera, FORWARD_PASS);
            s3d_render_copy(screen->pixels);
            SDL_Flip(screen);
        }
        else {
            // skip frame
        }

        int time_to_wait = time_delta - (SDL_GetTicks() - last_ticks);
        if (time_to_wait > 0)
            SDL_Delay(time_to_wait);
	}

    printf("Camera yaw %.5f, pitch %.5f\n", camera.yaw, camera.pitch);
    print_vec3(camera.position, "Camera position");

    mesh_free_obj(obj);
    camera_deinit(&camera);

    s3d_deinit();

    return 0;
}