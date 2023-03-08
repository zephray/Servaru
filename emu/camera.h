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
#pragma once

typedef struct {
    MAT4 projection_matrix;
    MAT4 view_matrix;
    MAT4 inverse_view_matrix;
    MAT4 projection_view_matrix;

    VEC4 frustum_planes[6];

    VEC3 position;
    VEC3 front;
    VEC3 up;
    VEC3 right;
    VEC3 world_up;

    float yaw;
    float pitch;
    float z_near;
    float z_far;

    bool look_at_origin;
} CAMERA;

typedef enum {
    M_FORWARD,
    M_BACKWARD,
    M_LEFT,
    M_RIGHT,
    M_UP,
    M_DOWN
} MOVEMENT;

void camera_init(CAMERA *camera);
void camera_deinit(CAMERA *camera);
void camera_update(CAMERA *camera);
void camera_move(CAMERA *camera, MOVEMENT movement, float distance);
void camera_set_position(CAMERA *camera, float x, float y, float z);
void camera_rotate(CAMERA *camera, float x, float y);
void camera_set_projection(CAMERA *camera, float fov, float aspect, float z_near, float z_far);
void camera_set_ortho_projection(CAMERA *camera, float size, float aspect, float z_near, float z_far);
void camera_toggle_lookat(CAMERA *camera);
