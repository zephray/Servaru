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
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "engine.h"

char *load_string_file(const char *path) {
	FILE *fp = fopen(path, "r");
	assert(fp);
	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *buffer = malloc(size + 1);
	assert(buffer);
	size_t count = fread(buffer, 1, size, fp);
	assert(count == size);
	fclose(fp);
	buffer[size] = '\0';
	return buffer;
}

char *strdupcat(char *a, char *b) {
    size_t lena = strlen(a);
    size_t lenb = strlen(b);
    char *dst = malloc(lena + lenb + 1);
    memcpy(&dst[0], a, lena);
    memcpy(&dst[lena], b, lenb);
    dst[lena + lenb] = '\0';
    return dst;
}

int line_to_tokens(char *line, char delim, char ***tokens) {
    size_t len = strlen(line);
    if (len == 0) return 0;
    size_t num_tokens = 0;
    bool intoken = false;
    for (size_t i = 0; i < len; i++) {
        if (line[i] == delim) {
            if (intoken)
                num_tokens++;
            intoken = false;
        }
        else {
            intoken = true;
        }
    }
    if (intoken)
        num_tokens++;
	
    if (num_tokens == 0)
        return 0;

    char **buffer = malloc(num_tokens * sizeof(char *));
    size_t li = 0;
	num_tokens = 0;
    for (size_t i = 0; i < len; i++) {
        if (line[i] == delim) {
            if (intoken) {
                int token_len = i - li;
                buffer[num_tokens] = malloc(token_len + 1);
                memcpy(buffer[num_tokens], &line[li], token_len);
                buffer[num_tokens][token_len] = '\0';
                num_tokens++;
            }
            intoken = false;
        }
        else {
            if (!intoken)
                li = i;
            intoken = true;
        }
    }
    if (intoken) {
        size_t token_len = len - li;
        buffer[num_tokens] = malloc(token_len + 1);
        memcpy(buffer[num_tokens], &line[li], token_len);
        buffer[num_tokens][token_len] = '\0';
        num_tokens++;
    }

    *tokens = buffer;
    return num_tokens;
}

void free_tokens(char **tokens, int num_tokens) {
    for (int i = 0; i < num_tokens; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

void ra_init(RESIZABLE_ARRAY *ra, size_t element_size) {
	size_t initial_size = 256 / element_size;
	if (initial_size == 0) initial_size = 1;
	ra->allocated_size = initial_size;
	ra->element_size = element_size;
	ra->used_size = 0;
	ra->buf = malloc(initial_size * element_size);
	assert(ra->buf);
}

void ra_deinit(RESIZABLE_ARRAY *ra) {
	free(ra->buf);
	ra->allocated_size = 0;
}

void ra_push(RESIZABLE_ARRAY *ra, void *val) {
	if ((ra->used_size + 1) > ra->allocated_size) {
		ra->allocated_size *= 2;
		ra->buf = realloc(ra->buf, ra->allocated_size * ra->element_size);
		assert(ra->buf);
	}
	memcpy((void *)((intptr_t)ra->buf + ra->used_size * ra->element_size), val,
			ra->element_size);
	ra->used_size++;
}

void ra_downsize(RESIZABLE_ARRAY *ra) {
	ra->allocated_size = ra->used_size;
	ra->buf = realloc(ra->buf, ra->allocated_size * ra->element_size);
	assert(ra->buf);
}

void print_float(float val, const char *name) {
    printf("%s: %.5f\n", name, val);
}

void print_vec4(VEC4 val, const char *name) {
    printf("%s: %.5f, %.5f, %.5f, %.5f\n", name, val.x, val.y, val.z, val.w);
}

void print_vec3(VEC3 val, const char *name) {
    printf("%s: %.5f, %.5f, %.5f\n", name, val.x, val.y, val.z);
}

void print_vec2(VEC2 val, const char *name) {
    printf("%s: %.5f, %.5f\n", name, val.x, val.y);
}

void print_mat4(MAT4 *val, const char *name) {
	printf("%s:\n", name);
	printf("[%.3f\t%.3f\t%.3f\t%.3f]\n", val->val[0][0], val->val[0][1], val->val[0][2], val->val[0][3]);
	printf("[%.3f\t%.3f\t%.3f\t%.3f]\n", val->val[1][0], val->val[1][1], val->val[1][2], val->val[1][3]);
	printf("[%.3f\t%.3f\t%.3f\t%.3f]\n", val->val[2][0], val->val[2][1], val->val[2][2], val->val[2][3]);
	printf("[%.3f\t%.3f\t%.3f\t%.3f]\n", val->val[3][0], val->val[3][1], val->val[3][2], val->val[3][3]);
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
void render_quad() {
    //
}
