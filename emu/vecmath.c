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
#include <stdio.h>
#include <math.h>
#include "vecmath.h"

VEC2 vec2_sub(VEC2 r1, VEC2 r2) {
    VEC2 result;
    result.x = r1.x - r2.x;
    result.y = r1.y - r2.y;
    return result;
}

VEC3 vec3_add(VEC3 r1, VEC3 r2) {
    VEC3 result;
    result.x = r1.x + r2.x;
    result.y = r1.y + r2.y;
    result.z = r1.z + r2.z;
    return result;
}

VEC3 vec3_adds(VEC3 r1, float r2) {
    VEC3 result;
    result.x = r1.x + r2;
    result.y = r1.y + r2;
    result.z = r1.z + r2;
    return result;
}

VEC3 vec3_sub(VEC3 r1, VEC3 r2) {
    VEC3 result;
    result.x = r1.x - r2.x;
    result.y = r1.y - r2.y;
    result.z = r1.z - r2.z;
    return result;
}

VEC3 vec3_subs(VEC3 r1, float r2) {
    VEC3 result;
    result.x = r1.x - r2;
    result.y = r1.y - r2;
    result.z = r1.z - r2;
    return result;
}

VEC3 vec3_div(VEC3 r1, float r2) {
    VEC3 result;
    result.x = r1.x / r2;
    result.y = r1.y / r2;
    result.z = r1.z / r2;
    return result;
}

VEC3 vec3_scale(VEC3 r1, float r2) {
    VEC3 result;
    result.x = r1.x * r2;
    result.y = r1.y * r2;
    result.z = r1.z * r2;
    return result;
}

VEC3 vec3_normalize(VEC3 r) {
    return vec3_div(r, vec3_length(r));
}

VEC3 vec3_cross(VEC3 r1, VEC3 r2) {
    VEC3 result = {
        r1.y * r2.z - r1.z * r2.y,
        r1.z * r2.x - r1.x * r2.z,
        r1.x * r2.y - r1.y * r2.x
    };
    return result;
}

float vec3_dot(VEC3 r1, VEC3 r2) {
    return r1.x * r2.x + r1.y * r2.y + r1.z * r2.z;
}

float vec3_length(VEC3 r) {
    return sqrtf(r.x * r.x + r.y * r.y + r.z * r.z);
}

VEC4 vec4_add(VEC4 r1, VEC4 r2) {
    VEC4 result = {
        r1.x + r2.x,
        r1.y + r2.y,
        r1.z + r2.z,
        r1.w + r2.w
    };
    return result;
}

VEC4 vec4_sub(VEC4 r1, VEC4 r2) {
    VEC4 result = {
        r1.x - r2.x,
        r1.y - r2.y,
        r1.z - r2.z,
        r1.w - r2.w
    };
    return result;
}

VEC4 vec4_mult(VEC4 r1, VEC4 r2) {
    VEC4 result = {
        r1.x * r2.x,
        r1.y * r2.y,
        r1.z * r2.z,
        r1.w * r2.w
    };
    return result;
}

float vec4_dot(VEC4 r1, VEC4 r2) {
    return r1.x * r2.x + r1.y * r2.y + r1.z * r2.z + r1.w * r2.w;
}

VEC4 vec4_lerp(float factor, VEC4 r1, VEC4 r2) {
    VEC4 result;
    result.x = r1.x * factor + r2.x * (1.f - factor);
    result.y = r1.y * factor + r2.y * (1.f - factor);
    result.z = r1.z * factor + r2.z * (1.f - factor);
    result.w = r1.w * factor + r2.w * (1.f - factor);
    return result;
}

MAT4 upper_mat3(MAT4 r) {
    r.val[0][3] = 0.f;
    r.val[1][3] = 0.f;
    r.val[2][3] = 0.f;
    r.val[3][0] = 0.f;
    r.val[3][1] = 0.f;
    r.val[3][2] = 0.f;
    r.val[3][3] = 0.f;
    return r;
}

MAT4 mat4_identity() {
    MAT4 result = {0};
    result.val[0][0] = 1.f;
    result.val[1][1] = 1.f;
    result.val[2][2] = 1.f;
    result.val[3][3] = 1.f;
    return result;
}

static MAT4 mat4_from_vec(VEC4 a, VEC4 b, VEC4 c, VEC4 d) {
    MAT4 r;
    r.val[0][0] = a.x; r.val[0][1] = a.y; r.val[0][2] = a.z; r.val[0][3] = a.w;
    r.val[1][0] = b.x; r.val[1][1] = b.y; r.val[1][2] = b.z; r.val[1][3] = b.w;
    r.val[2][0] = c.x; r.val[2][1] = c.y; r.val[2][2] = c.z; r.val[2][3] = c.w;
    r.val[3][0] = d.x; r.val[3][1] = d.y; r.val[3][2] = d.z; r.val[3][3] = d.w;
    return r;
}

MAT4 mat4_scale(MAT4 r1, float r2) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            r1.val[i][j] *= r2;
        }
    }
    return r1;
}

MAT4 mat4_multiply(MAT4 r1, MAT4 r2) {
    MAT4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.val[i][j] = 0;
        }
    }

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            for (int k = 0; k < 4; ++k) {
                result.val[i][j] += r2.val[i][k] * r1.val[k][j];
            }
        }
    }
    return result;
}

VEC4 mat4_multiply_by_vec4(MAT4 r1, VEC4 r2) {
    float result[4];
    float r2v[4] = {r2.x, r2.y, r2.z, r2.w};
    for (int i = 0; i < 4; i++) {
        result[i] = 0;
        for (int j = 0; j < 4; j++) {
            result[i] += r1.val[j][i] * r2v[j];
        }
    }
    VEC4 l = {result[0], result[1], result[2], result[3]};
    return l;
}

MAT4 mat4_inverse(MAT4 r) {
    float coef00 = r.val[2][2] * r.val[3][3] - r.val[3][2] * r.val[2][3];
    float coef02 = r.val[1][2] * r.val[3][3] - r.val[3][2] * r.val[1][3];
    float coef03 = r.val[1][2] * r.val[2][3] - r.val[2][2] * r.val[1][3];

    float coef04 = r.val[2][1] * r.val[3][3] - r.val[3][1] * r.val[2][3];
    float coef06 = r.val[1][1] * r.val[3][3] - r.val[3][1] * r.val[1][3];
    float coef07 = r.val[1][1] * r.val[2][3] - r.val[2][1] * r.val[1][3];

    float coef08 = r.val[2][1] * r.val[3][2] - r.val[3][1] * r.val[2][2];
    float coef10 = r.val[1][1] * r.val[3][2] - r.val[3][1] * r.val[1][2];
    float coef11 = r.val[1][1] * r.val[2][2] - r.val[2][1] * r.val[1][2];

    float coef12 = r.val[2][0] * r.val[3][3] - r.val[3][0] * r.val[2][3];
    float coef14 = r.val[1][0] * r.val[3][3] - r.val[3][0] * r.val[1][3];
    float coef15 = r.val[1][0] * r.val[2][3] - r.val[2][0] * r.val[1][3];

    float coef16 = r.val[2][0] * r.val[3][2] - r.val[3][0] * r.val[2][2];
    float coef18 = r.val[1][0] * r.val[3][2] - r.val[3][0] * r.val[1][2];
    float coef19 = r.val[1][0] * r.val[2][2] - r.val[2][0] * r.val[1][2];
    
    float coef20 = r.val[2][0] * r.val[3][1] - r.val[3][0] * r.val[2][1];
    float coef22 = r.val[1][0] * r.val[3][1] - r.val[3][0] * r.val[1][1];
    float coef23 = r.val[1][0] * r.val[2][1] - r.val[2][0] * r.val[1][1];

	VEC4 fac0 = {coef00, coef00, coef02, coef03};
	VEC4 fac1 = {coef04, coef04, coef06, coef07};
	VEC4 fac2 = {coef08, coef08, coef10, coef11};
	VEC4 fac3 = {coef12, coef12, coef14, coef15};
	VEC4 fac4 = {coef16, coef16, coef18, coef19};
	VEC4 fac5 = {coef20, coef20, coef22, coef23};

    VEC4 vec0 = {r.val[1][0], r.val[0][0], r.val[0][0], r.val[0][0]};
    VEC4 vec1 = {r.val[1][1], r.val[0][1], r.val[0][1], r.val[0][1]};
    VEC4 vec2 = {r.val[1][2], r.val[0][2], r.val[0][2], r.val[0][2]};
    VEC4 vec3 = {r.val[1][3], r.val[0][3], r.val[0][3], r.val[0][3]};

    VEC4 inv0 = vec4_add(vec4_sub(vec4_mult(vec1, fac0), vec4_mult(vec2, fac1)), vec4_mult(vec3, fac2));
    VEC4 inv1 = vec4_add(vec4_sub(vec4_mult(vec0, fac0), vec4_mult(vec2, fac3)), vec4_mult(vec3, fac4));
    VEC4 inv2 = vec4_add(vec4_sub(vec4_mult(vec0, fac1), vec4_mult(vec1, fac3)), vec4_mult(vec3, fac5));
    VEC4 inv3 = vec4_add(vec4_sub(vec4_mult(vec0, fac2), vec4_mult(vec1, fac4)), vec4_mult(vec2, fac5));

    VEC4 sign_a = {+1.f, -1.f, +1.f, -1.f};
    VEC4 sign_b = {-1.f, +1.f, -1.f, +1.f};

    MAT4 inverse = mat4_from_vec(
        vec4_mult(inv0, sign_a), vec4_mult(inv1, sign_b), vec4_mult(inv2, sign_a), vec4_mult(inv3, sign_b)
    );

    VEC4 row0 = {inverse.val[0][0], inverse.val[1][0], inverse.val[2][0], inverse.val[3][0]};
    VEC4 col0 = {r.val[0][0], r.val[0][1], r.val[0][2], r.val[0][3]};
    VEC4 dot0 = vec4_mult(col0, row0);
    float dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);
    float one_over_determinant = 1.0f / dot1;

    return mat4_scale(inverse, one_over_determinant);
}

MAT4 mat4_transpose(MAT4 r) {
    MAT4 result;
    result.val[0][0] = r.val[0][0];
    result.val[0][1] = r.val[1][0];
    result.val[0][2] = r.val[2][0];
    result.val[0][3] = r.val[3][0];

    result.val[1][0] = r.val[0][1];
    result.val[1][1] = r.val[1][1];
    result.val[1][2] = r.val[2][1];
    result.val[1][3] = r.val[3][1];

    result.val[2][0] = r.val[0][2];
    result.val[2][1] = r.val[1][2];
    result.val[2][2] = r.val[2][2];
    result.val[2][3] = r.val[3][2];
    
    result.val[3][0] = r.val[0][3];
    result.val[3][1] = r.val[1][3];
    result.val[3][2] = r.val[2][3];
    result.val[3][3] = r.val[3][3];
    return result;
}

MAT4 perspective(float fov, float aspect, float z_near, float z_far) {
    MAT4 result = {0};
    float tanHalfFov = tanf(fov / 2.0f);

    result.val[0][0] = 1.0f / (aspect * tanHalfFov);
    result.val[1][1] = 1.0f / tanHalfFov;
    result.val[2][2] = - (z_far + z_near) / (z_far - z_near);
    result.val[2][3] = - 1.0f;
    result.val[3][2] = - (2.0f * z_far * z_near) / (z_far - z_near);

    return result;
}

MAT4 ortho(float left, float right, float bottom, float top, float z_near, float z_far) {
    MAT4 result = {0};
    result.val[0][0] = 2.0f / (right - left);
    result.val[1][1] = 2.0f / (top - bottom);
    result.val[2][2] = - 2.0f / (z_far - z_near);
    result.val[3][0] = - (right + left) / (right - left);
    result.val[3][1] = - (top + bottom) / (top - bottom);
    result.val[3][2] = - (z_far + z_near) / (z_far - z_near);
    result.val[3][3] = 1.0f;
    return result;
}

MAT4 look_at(VEC3 eye, VEC3 center, VEC3 up) {
    VEC3 f = vec3_normalize(vec3_sub(center, eye));
    VEC3 s = vec3_normalize(vec3_cross(f, up));
    VEC3 u = vec3_cross(s, f);
    MAT4 result = {0};

    result.val[0][0] = s.x;
    result.val[1][0] = s.y;
    result.val[2][0] = s.z;
    result.val[0][1] = u.x;
    result.val[1][1] = u.y;
    result.val[2][1] = u.z;
    result.val[0][2] =-f.x;
    result.val[1][2] =-f.y;
    result.val[2][2] =-f.z;
    result.val[3][0] =-vec3_dot(s, eye);
    result.val[3][1] =-vec3_dot(u, eye);
    result.val[3][2] = vec3_dot(f, eye);
    result.val[3][3] = 1.f;

    return result;
}
