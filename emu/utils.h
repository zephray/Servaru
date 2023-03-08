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
    size_t allocated_size;
    size_t used_size;
    size_t element_size;
    void *buf;
} RESIZABLE_ARRAY;

char *load_string_file(const char *path);
char *strdupcat(char *a, char *b);
int line_to_tokens(char *line, char delim, char ***tokens);
void free_tokens(char **tokens, int num_tokens);

// Resizable array
void ra_init(RESIZABLE_ARRAY *ra, size_t element_size);
void ra_deinit(RESIZABLE_ARRAY *ra);
void ra_push(RESIZABLE_ARRAY *ra, void *val);
void ra_downsize(RESIZABLE_ARRAY *ra);

// Prints
void print_float(float val, const char *name);
void print_vec4(VEC4 val, const char *name);
void print_vec3(VEC3 val, const char *name);
void print_vec2(VEC2 val, const char *name);
void print_mat4(MAT4 *val, const char *name);

void render_quad();