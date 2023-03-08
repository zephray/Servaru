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
    float x;
    float y;
} VEC2;

typedef struct {
    float x;
    float y;
    float z;
} VEC3;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} VEC4;

typedef struct {
    float val[3][3];
} MAT3;

typedef struct {
    float val[4][4];
} MAT4;

#define RADIAN(x) ((x) * 0.01745329251994329f)
#define DEGREE(x) ((x) * 57.2957795130823208f)

VEC2 vec2_sub(VEC2 r1, VEC2 r2);

VEC3 vec3_add(VEC3 r1, VEC3 r2);
VEC3 vec3_adds(VEC3 r1, float r2);
VEC3 vec3_sub(VEC3 r1, VEC3 r2);
VEC3 vec3_subs(VEC3 r1, float r2);
VEC3 vec3_div(VEC3 r1, float r2);
VEC3 vec3_scale(VEC3 r1, float r2);
VEC3 vec3_normalize(VEC3 r);
VEC3 vec3_cross(VEC3 r1, VEC3 r2);
float vec3_dot(VEC3 r1, VEC3 r2);
float vec3_length(VEC3 r);
VEC4 vec4_add(VEC4 r1, VEC4 r2);
VEC4 vec4_sub(VEC4 r1, VEC4 r2);
VEC4 vec4_mult(VEC4 r1, VEC4 r2);
float vec4_dot(VEC4 r1, VEC4 r2);
VEC4 vec4_lerp(float factor, VEC4 r1, VEC4 r2);
MAT4 upper_mat3(MAT4 r);
MAT4 mat4_identity();
MAT4 mat4_scale(MAT4 r1, float r2);
MAT4 mat4_inverse(MAT4 r);
MAT4 mat4_transpose(MAT4 r);
MAT4 mat4_multiply(MAT4 r1, MAT4 r2);
VEC4 mat4_multiply_by_vec4(MAT4 r1, VEC4 r2);

MAT4 perspective(float fov, float aspect, float z_near, float z_far);
MAT4 ortho(float left, float right, float bottom, float top, float z_near, float z_far);
MAT4 look_at(VEC3 eye, VEC3 center, VEC3 up);
