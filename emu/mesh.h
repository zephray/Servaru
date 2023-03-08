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

typedef enum {
    DEPTH_PASS, // Depth pass without alpha for shadow map
    DEPTH_PREPASS, // Depth pass with alpha for scene
    GEOMETRY_PASS, // Gbuffer pass for deferred shading
    FORWARD_PASS // Forward shading shader
} RENDERPASS;

typedef struct {
    char *name;

    TEXTURE *tex_ambient;
    TEXTURE *tex_diffuse;
    TEXTURE *tex_specular;
    TEXTURE *tex_alpha;
    TEXTURE *tex_displace;

    VEC3 ambient;
    VEC3 diffuse;
    VEC3 specular;

    float specular_exponent;
    float optical_density;
    float dissolve;
} MATERIAL;

typedef struct {
    VEC3 position;
//    VEC3 normal;
    VEC2 tex_coord;
} VERTEX;

typedef struct {
    char *name;

    VERTEX *vertices;
    size_t num_vertices;
    uint32_t *indices;
    size_t num_indices;
    MATERIAL *material;

    uint32_t vao;
    uint32_t vbo;
    uint32_t ebo;
} MESH;

typedef struct {
    MESH *meshes;
    size_t num_meshes;
    VEC4 *bounding_spheres;
    // For tracking purposes
    MATERIAL *materials;
    size_t num_materials;
    TEXTURE *textures;
    size_t num_textures;
    SHADER *depth_shader;
    SHADER *depth_alpha_shader;
    SHADER *gbuffer_shader;
    SHADER *forward_shader;
} OBJ;

OBJ *mesh_load_obj(char *path, char *fname, float scale);
void mesh_free_obj(OBJ *obj);
void mesh_dump(MESH *mesh);
void mesh_init(MESH *mesh);
size_t mesh_size(MESH *mesh);
void mesh_render_obj(OBJ *obj, CAMERA *camera, RENDERPASS renderpass);
