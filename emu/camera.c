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
#include <math.h>
#include <string.h>
#include "engine.h"

void camera_init(CAMERA *camera) {
    memset(camera, 0, sizeof(CAMERA));
    camera->world_up.x = 0.0f;
    camera->world_up.y = 1.0f;
    camera->world_up.z = 0.0f;
    camera_update(camera);
}

void camera_deinit(CAMERA *camera) {
    return;
}

void camera_update(CAMERA *camera) {
    camera->front.x = cosf(RADIAN(camera->yaw)) * cosf(RADIAN(camera->pitch));
    camera->front.y = sinf(RADIAN(camera->pitch));
    camera->front.z = sinf(RADIAN(camera->yaw)) * cosf(RADIAN(camera->pitch));
    /*camera->front.x = -camera->position.x;
    camera->front.y = -camera->position.y;
    camera->front.z = -camera->position.z;*/
    //print_vec3(camera->position, "Position");
    //print_vec3(camera->front, "Front");

    camera->front = vec3_normalize(camera->front);
    camera->right = vec3_normalize(vec3_cross(camera->front, camera->world_up));
    camera->up = vec3_normalize(vec3_cross(camera->right, camera->front));
    //print_vec3(camera->right, "Right");
    //print_vec3(camera->up, "Up");

    if (camera->look_at_origin) {
        VEC3 zero = {0.f, 0.f, 0.f};
        camera->view_matrix = look_at(camera->position, zero, camera->up);
    }
    else {
        camera->view_matrix = look_at(camera->position,
                vec3_add(camera->position, camera->front), camera->up);
    }
    camera->inverse_view_matrix = mat4_inverse(camera->view_matrix);
    camera->projection_view_matrix = mat4_multiply(camera->projection_matrix,
            camera->view_matrix);
    //print_mat4(&camera->projection_view_matrix, "projection_view");

    //print_mat4(&camera->view_matrix, "View matrix");

    MAT4 frustum_matrix = mat4_transpose(mat4_multiply(camera->projection_matrix, camera->view_matrix));
    float sign = 1.0f;
    for (int i = 0; i < 6; i++) {
        const int idx = (i & ~1) >> 1;
        camera->frustum_planes[i].x = frustum_matrix.val[3][0] + (sign * frustum_matrix.val[idx][0]);
        camera->frustum_planes[i].y = frustum_matrix.val[3][1] + (sign * frustum_matrix.val[idx][1]);
        camera->frustum_planes[i].z = frustum_matrix.val[3][2] + (sign * frustum_matrix.val[idx][2]);
        camera->frustum_planes[i].w = frustum_matrix.val[3][3] + (sign * frustum_matrix.val[idx][3]);
        VEC3 plane_xyz = {
            camera->frustum_planes[i].x,
            camera->frustum_planes[i].y,
            camera->frustum_planes[i].z
        };
        float length = vec3_length(plane_xyz);
        camera->frustum_planes[i].x /= length;
        camera->frustum_planes[i].y /= length;
        camera->frustum_planes[i].z /= length;
        camera->frustum_planes[i].w /= length;
        sign *= -1.0f;
    }
}

void camera_move(CAMERA *camera, MOVEMENT movement, float distance) {
    switch(movement) {
    case M_FORWARD:
        camera->position = vec3_add(camera->position, vec3_scale(camera->front, distance));
        break;
    case M_BACKWARD:
        camera->position = vec3_sub(camera->position, vec3_scale(camera->front, distance));
        break;
    case M_LEFT:
        camera->position = vec3_sub(camera->position, vec3_scale(camera->right, distance));
        break;
    case M_RIGHT:
        camera->position = vec3_add(camera->position, vec3_scale(camera->right, distance));
        break;
    case M_UP:
        camera->position = vec3_add(camera->position, vec3_scale(camera->up, distance));
        break;
    case M_DOWN:
        camera->position = vec3_sub(camera->position, vec3_scale(camera->up, distance));
        break;
    }
    camera_update(camera);
}

void camera_rotate(CAMERA *camera, float x, float y) {
    camera->yaw += x;
    camera->pitch += y;
    if (camera->pitch > 89.0f)
        camera->pitch = 89.0f;
    if (camera->pitch < -89.0f)
        camera->pitch = -89.0f;
    if (camera->yaw < -0.0f)
        camera->yaw += 360.0f;
    if (camera->yaw > 360.0f)
        camera->yaw -= 360.0f;
    camera_update(camera);
}

void camera_set_position(CAMERA *camera, float x, float y, float z) {
    camera->position.x = x;
    camera->position.y = y;
    camera->position.z = z;
    camera_update(camera);
}

void camera_set_projection(CAMERA *camera, float fov, float aspect, float z_near, float z_far) {
    camera->z_near = z_near;
    camera->z_far = z_far;
    camera->projection_matrix = perspective(fov, aspect, z_near, z_far);
    //camera->projection_matrix = mat4_identity();
    //print_mat4(&camera->projection_matrix, "Projection matrix");
}

void camera_set_ortho_projection(CAMERA *camera, float size, float aspect, float z_near, float z_far) {
    camera->z_near = z_near;
    camera->z_far = z_far;
    camera->projection_matrix = ortho(-size, size, -size, size, z_near, z_far);
}

void camera_toggle_lookat(CAMERA *camera) {
    camera->look_at_origin = !camera->look_at_origin;
}