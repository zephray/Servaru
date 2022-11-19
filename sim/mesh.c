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
#include <assert.h>
#include <math.h>
#include <string.h>
#include "hashmap.h"
#include "engine.h"

//#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(...) do{ printf( __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif

typedef struct {
    struct hashmap_s material_map;
    RESIZABLE_ARRAY materials;
    struct hashmap_s texture_map;
    RESIZABLE_ARRAY textures;
} MTL;

void mesh_calculate_normal(MESH *mesh);
void mesh_calculate_tangent(MESH *mesh);

static VEC2 parse_vec2(char **tokens) {
    VEC2 result;
    result.x = atof(tokens[1]);
    result.y = atof(tokens[2]);
    return result;
}

static VEC3 parse_vec3(char **tokens) {
    VEC3 result;
    result.x = atof(tokens[1]);
    result.y = atof(tokens[2]);
    result.z = atof(tokens[3]);
    return result;
}

static void print_vertex(VERTEX *vertex) {
    print_vec3(vertex->position, "Position");
    //print_vec3(vertex->normal, "Normal");
    print_vec2(vertex->tex_coord, "Tex Coord");
}

static uint32_t parse_vertex(RESIZABLE_ARRAY *vertices,
        struct hashmap_s *vertex_hashmap,
        char *str, RESIZABLE_ARRAY *positions,
        RESIZABLE_ARRAY *tex_coords, RESIZABLE_ARRAY *normals) {
    // Check if already exist
    DEBUG_PRINT("Checking vertex %s\n", str);
    void * const element = hashmap_get(vertex_hashmap, str, strlen(str));
    if (NULL != element) {
        DEBUG_PRINT("Vertex found at ID %u\n", (uint32_t)(intptr_t)element - 1);
        return (uint32_t)(intptr_t)element - 1;
    }

    char **tokens;
    int num_tokens = line_to_tokens(str, '/', &tokens);
    assert((num_tokens >= 1) && (num_tokens <= 3));
    VERTEX result;
    int ipos = atoi(tokens[0]);
    if (ipos < 0) ipos = positions->used_size + ipos; else ipos -= 1;
    result.position = ((VEC3 *)positions->buf)[ipos];
    if (num_tokens >= 2) {
        int itex = atoi(tokens[1]);
        if (itex < 0) itex = tex_coords->used_size + itex; else itex -= 1;
        result.tex_coord = ((VEC2 *)tex_coords->buf)[itex];
    }
    else {
        result.tex_coord.x = 0;
        result.tex_coord.y = 0;
    }
    /*if (num_tokens == 3) {
        int inom = atoi(tokens[2]);
        if (inom < 0) inom = normals->used_size + inom; else inom -= 1;
        result.normal = ((VEC3 *)normals->buf)[inom];
    }
    else {
        result.normal.x = 0;
        result.normal.y = 0;
        result.normal.z = 0;
    }*/

    ra_push(vertices, &result);
    uint32_t id = vertices->used_size; // 1-indexed
    char *key = strdup(str);
    assert(hashmap_put(vertex_hashmap, key, strlen(key), (void *)(intptr_t)id) == 0);
    DEBUG_PRINT("New vertex allocated at ID %u\n", id - 1);
    free_tokens(tokens, num_tokens);
    return id - 1;
}

static void parse_triangle(RESIZABLE_ARRAY *indices, RESIZABLE_ARRAY *vertices,
        struct hashmap_s *vertex_hashmap,
        char *a, char *b, char *c, RESIZABLE_ARRAY *positions,
        RESIZABLE_ARRAY *tex_coords, RESIZABLE_ARRAY *normals) {
    DEBUG_PRINT("Parse triangle %s %s %s\n", a, b, c);
    uint32_t v1 = parse_vertex(vertices, vertex_hashmap, a,
            positions, tex_coords, normals);
    uint32_t v2 = parse_vertex(vertices, vertex_hashmap, b,
            positions, tex_coords, normals);
    uint32_t v3 = parse_vertex(vertices, vertex_hashmap, c,
            positions, tex_coords, normals);
    ra_push(indices, &v1);
    ra_push(indices, &v2);
    ra_push(indices, &v3);
}

static void parse_mesh(RESIZABLE_ARRAY *vertices, RESIZABLE_ARRAY *indices,
        RESIZABLE_ARRAY *meshes, char *name, MATERIAL *material) {
    // Write vertices and indices into the mesh
    MESH mesh;
    mesh.name = name;
    mesh.material = material;
    ra_downsize(vertices);
    mesh.num_vertices = vertices->used_size;
    mesh.vertices = vertices->buf;
    ra_downsize(indices);
    mesh.num_indices = indices->used_size;
    mesh.indices = indices->buf;
    ra_push(meshes, &mesh);
}

static int hashmap_free_key(void* const context, struct hashmap_element_s* const e) {
    free((void *)e->key);
    return 0;
}

static void clear_hashmap_key(struct hashmap_s *hashmap) {
    hashmap_iterate_pairs(hashmap, hashmap_free_key, NULL);
    hashmap_destroy(hashmap);
}

// Return the ID, 1 indexed!
static size_t mesh_load_texture(MTL *mtl, char *path, char *fname) {
    void * const element = hashmap_get(&mtl->texture_map, fname, strlen(fname));
    if (NULL != element) {
        DEBUG_PRINT("Texture found at ID %u\n", (uint32_t)(intptr_t)element - 1);
        return (size_t)(intptr_t)element;
    }
    char *texname = strdupcat(path, fname);
    printf("Loading %s\n", texname);
    TEXTURE texture = texture_load(texname, fname);
    free(texname);
    ra_push(&mtl->textures, &texture);
    assert(hashmap_put(&mtl->texture_map, texture.name, strlen(texture.name),
            (void *)(intptr_t)(mtl->textures.used_size)) == 0);
    return (mtl->textures.used_size);
}

static void mesh_load_mtl(MTL *mtl, char *path, char *fname) {
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t rl;
    MATERIAL new_material;
    bool in_material = false;

    ra_init(&mtl->materials, sizeof(MATERIAL));
    assert(hashmap_create(2, &mtl->material_map) == 0);
    ra_init(&mtl->textures, sizeof(TEXTURE));
    assert(hashmap_create(2, &mtl->texture_map) == 0);

    memset(&new_material, 0, sizeof(MATERIAL));

    char *mtlname = strdupcat(path, fname);
    printf("Loading %s\n", mtlname);
    fp = fopen(mtlname, "r");
    free(mtlname);
    assert(fp);

    while ((rl = getline(&line, &len, fp)) != -1) {
        for (ssize_t i = 0; i < rl; i++) {
            if ((line[i] == '\r') || (line[i] == '\n'))
                line[i] = '\0';
        }
        char **tokens;
        int num_tokens = line_to_tokens(line, ' ', &tokens);
        if (num_tokens == 0) continue;

        if (strcmp(tokens[0], "newmtl") == 0) {
            if (in_material) {
                ra_push(&mtl->materials, &new_material);
                hashmap_put(&mtl->material_map, new_material.name,
                        strlen(new_material.name),
                        (void *)(intptr_t)(mtl->materials.used_size));
                DEBUG_PRINT("Assigning ID %ld to material %s\n",
                        mtl->materials.used_size - 1, new_material.name);
                memset(&new_material, 0, sizeof(MATERIAL));
            }
            in_material = true;
            new_material.name = strdup(tokens[1]);
        }
        else if (strcmp(tokens[0], "Ns") == 0) {
            new_material.specular_exponent = atof(tokens[1]);
        }
        else if (strcmp(tokens[0], "Ka") == 0) {
            new_material.ambient = parse_vec3(tokens);
        }
        else if (strcmp(tokens[0], "Kd") == 0) {
            new_material.diffuse = parse_vec3(tokens);
        }
        else if (strcmp(tokens[0], "Ks") == 0) {
            new_material.specular = parse_vec3(tokens);
        }
        else if (strcmp(tokens[0], "Ni") == 0) {
            new_material.optical_density = atof(tokens[1]);
        }
        else if (strcmp(tokens[0], "d") == 0) {
            new_material.dissolve = atof(tokens[1]);
        }
        else if (strcmp(tokens[0], "illum") == 0) {
            // Ignored
        }
        else if (strcmp(tokens[0], "map_Disp") == 0) {
            new_material.tex_displace = (TEXTURE *)mesh_load_texture(mtl, path, tokens[1]);
        }
        else if (strcmp(tokens[0], "map_Ka") == 0) {
            new_material.tex_ambient = (TEXTURE *)mesh_load_texture(mtl, path, tokens[1]);
        }
        else if (strcmp(tokens[0], "map_Kd") == 0) {
            new_material.tex_diffuse = (TEXTURE *)mesh_load_texture(mtl, path, tokens[1]);
        }
        else if (strcmp(tokens[0], "map_Ks") == 0) {
            new_material.tex_specular = (TEXTURE *)mesh_load_texture(mtl, path, tokens[1]);
        }
        else if (strcmp(tokens[0], "map_d") == 0) {
            new_material.tex_alpha = (TEXTURE *)mesh_load_texture(mtl, path, tokens[1]);
        }
        else {
            if (strcmp(tokens[0], "#") != 0)
                printf("Unhandled directive %s\n", tokens[0]);
        }
        free_tokens(tokens, num_tokens);
    }
    if (line)
        free(line);

    fclose(fp);

    if (in_material) {
        ra_push(&mtl->materials, &new_material);
        hashmap_put(&mtl->material_map, new_material.name,
                strlen(new_material.name),
                (void *)(intptr_t)(mtl->materials.used_size));
        DEBUG_PRINT("Assigning ID %ld to material %s\n",
                mtl->materials.used_size - 1, new_material.name);
    }
}

// Return ID, 1 indexed!
static size_t mesh_find_material(MTL *mtl, char *name) {
    void * const element = hashmap_get(&mtl->material_map, name, strlen(name));
    if (NULL != element) {
        DEBUG_PRINT("Material found at ID %u\n", (uint32_t)(intptr_t)element - 1);
        return (size_t)(intptr_t)element;
    }
    printf("Unable to find material %s\n", name);
    return 0;
}

// Return the number of meshes loaded
OBJ *mesh_load_obj(char *path, char *fname, float scale) {
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t rl;
    RESIZABLE_ARRAY obj_vertices;
    RESIZABLE_ARRAY obj_positions;
    RESIZABLE_ARRAY obj_tex_coords;
    RESIZABLE_ARRAY obj_normals;
    RESIZABLE_ARRAY obj_indices;
    RESIZABLE_ARRAY obj_meshes;
    struct hashmap_s vertex_hashmap;
    MTL mtl;
    MATERIAL *mesh_mtl = NULL;

    bool in_mesh = false;
    char *mesh_name = NULL;

    char *objname = strdupcat(path, fname);
    printf("Loading %s\n", objname);
    fp = fopen(objname, "r");
    free(objname);
    assert(fp);

    ra_init(&obj_vertices, sizeof(VERTEX));
    ra_init(&obj_positions, sizeof(VEC3));
    ra_init(&obj_tex_coords, sizeof(VEC2));
    ra_init(&obj_normals, sizeof(VEC3));
    ra_init(&obj_indices, sizeof(uint32_t));
    ra_init(&obj_meshes, sizeof(MESH));
    assert(hashmap_create(2, &vertex_hashmap) == 0);

    while ((rl = getline(&line, &len, fp)) != -1) {
        for (ssize_t i = 0; i < rl; i++) {
            if ((line[i] == '\r') || (line[i] == '\n'))
                line[i] = '\0';
        }
        char **tokens;
        int num_tokens = line_to_tokens(line, ' ', &tokens);
        if (num_tokens == 0) continue;

        if (strcmp(tokens[0], "vt") == 0) {
            VEC2 tex_coord = parse_vec2(tokens);
            ra_push(&obj_tex_coords, &tex_coord);
        }
        else if (strcmp(tokens[0], "v") == 0) {
            VEC3 position = parse_vec3(tokens);
            position = vec3_scale(position, scale);
            ra_push(&obj_positions, &position);
        }
        else if (strcmp(tokens[0], "vn") == 0) {
            VEC3 normal = parse_vec3(tokens);
            ra_push(&obj_normals, &normal);
        }
        else if (strcmp(tokens[0], "f") == 0) {
            if (num_tokens == 4) { // triangle
                parse_triangle(&obj_indices, &obj_vertices, &vertex_hashmap,
                        tokens[1], tokens[2], tokens[3],
                        &obj_positions, &obj_tex_coords, &obj_normals);
            }
            else if (num_tokens == 5) { // quad, break into 2 triangles
                parse_triangle(&obj_indices, &obj_vertices, &vertex_hashmap,
                        tokens[1], tokens[2], tokens[4],
                        &obj_positions, &obj_tex_coords, &obj_normals);
                parse_triangle(&obj_indices, &obj_vertices, &vertex_hashmap,
                        tokens[2], tokens[3], tokens[4],
                        &obj_positions, &obj_tex_coords, &obj_normals);
            }
        }
        else if (strcmp(tokens[0], "g") == 0) {
            if (in_mesh) {
                parse_mesh(&obj_vertices, &obj_indices, &obj_meshes, mesh_name, mesh_mtl);
                // Reset vertices/ indices array
                ra_init(&obj_vertices, sizeof(VERTEX));
                ra_init(&obj_indices, sizeof(uint32_t));
                clear_hashmap_key(&vertex_hashmap);
                assert(hashmap_create(2, &vertex_hashmap) == 0);
            }
            else {
                in_mesh = true;
            }
            mesh_name = strdup(tokens[1]);
        }
        else if (strcmp(tokens[0], "mtllib") == 0) {
            mesh_load_mtl(&mtl, path, tokens[1]);
        }
        else if (strcmp(tokens[0], "usemtl") == 0) {
            mesh_mtl = (MATERIAL *)mesh_find_material(&mtl, tokens[1]);
        }
        else {
            // Ignore comment, and object command
            if ((strcmp(tokens[0], "#") != 0) && (strcmp(tokens[0], "o") != 0))
                printf("Unhandled directive %s\n", tokens[0]);
        }
        free_tokens(tokens, num_tokens);
    }
    if (line)
        free(line);

    fclose(fp);

    if (!in_mesh)
        mesh_name = "Unnamed";

    parse_mesh(&obj_vertices, &obj_indices, &obj_meshes, mesh_name, mesh_mtl);

    /*if (obj_normals.used_size == 0) {
        printf("Model doesn't have normal, calculated result may be sub-optimal.\n");
        for (size_t i = 0; i < obj_meshes.used_size; i++) {
            mesh_calculate_normal(&((MESH *)obj_meshes.buf)[i]);
        }
    }*/
    
    // Free buffers. vertices/ indices buffers should not be freed
    ra_deinit(&obj_positions);
    ra_deinit(&obj_tex_coords);
    ra_deinit(&obj_normals);
    clear_hashmap_key(&vertex_hashmap);

    hashmap_destroy(&mtl.material_map);
    hashmap_destroy(&mtl.texture_map);

    OBJ *obj = malloc(sizeof(OBJ));

    ra_downsize(&obj_meshes);
    obj->meshes = (MESH *)obj_meshes.buf;
    obj->num_meshes = obj_meshes.used_size;
    ra_downsize(&mtl.materials);
    obj->materials = (MATERIAL *)mtl.materials.buf;
    obj->num_materials = mtl.materials.used_size;
    ra_downsize(&mtl.textures);
    obj->textures = (TEXTURE *)mtl.textures.buf;
    obj->num_textures = mtl.textures.used_size;

    // After resizing (addresses might have changed), relink textures, materials, and meshes
    for (size_t i = 0; i < obj->num_meshes; i++) {
        obj->meshes[i].material = ((size_t)obj->meshes[i].material != 0) ?
                &obj->materials[(size_t)obj->meshes[i].material - 1] : NULL;
    }
    for (size_t i = 0; i < obj->num_materials; i++) {
        obj->materials[i].tex_alpha = ((size_t)obj->materials[i].tex_alpha != 0) ?
                &obj->textures[(size_t)obj->materials[i].tex_alpha - 1] : NULL;
        obj->materials[i].tex_ambient =  ((size_t)obj->materials[i].tex_ambient != 0) ?
                &obj->textures[(size_t)obj->materials[i].tex_ambient - 1] : NULL;
        obj->materials[i].tex_diffuse =  ((size_t)obj->materials[i].tex_diffuse != 0) ?
                &obj->textures[(size_t)obj->materials[i].tex_diffuse - 1] : NULL;
        obj->materials[i].tex_displace = ((size_t)obj->materials[i].tex_displace != 0) ?
                &obj->textures[(size_t)obj->materials[i].tex_displace - 1] : NULL;
        obj->materials[i].tex_specular = ((size_t)obj->materials[i].tex_specular != 0) ?
                &obj->textures[(size_t)obj->materials[i].tex_specular - 1] : NULL;
    }

    // Update bounding spheres
    obj->bounding_spheres = malloc(sizeof(VEC4) * obj->num_meshes);
    for (size_t i = 0; i < obj->num_meshes; i++) {
        // Calculate AABB
        VEC3 aabb_min = obj->meshes[i].vertices[0].position;
        VEC3 aabb_max = obj->meshes[i].vertices[0].position;
        for (size_t j = 0; j < obj->meshes[i].num_vertices; j++) {
            aabb_min.x = fminf(obj->meshes[i].vertices[j].position.x, aabb_min.x);
            aabb_min.y = fminf(obj->meshes[i].vertices[j].position.y, aabb_min.y);
            aabb_min.z = fminf(obj->meshes[i].vertices[j].position.z, aabb_min.z);
            aabb_max.x = fmaxf(obj->meshes[i].vertices[j].position.x, aabb_min.x);
            aabb_max.y = fmaxf(obj->meshes[i].vertices[j].position.y, aabb_min.y);
            aabb_max.z = fmaxf(obj->meshes[i].vertices[j].position.z, aabb_min.z);
        }
        // Calculate bounding sphere
        VEC3 center = vec3_div(vec3_add(aabb_min, aabb_max), 2);
        float radius = 0;
        for (size_t j = 0; j < obj->meshes[i].num_vertices; j++) {
            float distance = vec3_length(vec3_sub(obj->meshes[i].vertices[j].position, center));
            radius = fmaxf(radius, distance);
        }
        obj->bounding_spheres[i].x = center.x;
        obj->bounding_spheres[i].y = center.y;
        obj->bounding_spheres[i].z = center.z;
        obj->bounding_spheres[i].w = radius;
    }

    return obj;
}

void mesh_free_obj(OBJ *obj) {
    for (size_t i = 0; i < obj->num_textures; i++) {
        texture_free(&obj->textures[i]);
    }
    free(obj->textures);
    for (size_t i = 0; i < obj->num_materials; i++) {
        free(obj->materials[i].name);
    }
    free(obj->materials);
    for (size_t i = 0; i < obj->num_meshes; i++) {
        free(obj->meshes[i].name);
        s3d_delete_vbo(obj->meshes[i].vbo);
        s3d_delete_ebo(obj->meshes[i].ebo);
        s3d_delete_vao(obj->meshes[i].vao);
        free(obj->meshes[i].vertices);
        free(obj->meshes[i].indices);
    }
    free(obj->meshes);
    free(obj);
}

void mesh_dump(MESH *mesh) {
    printf("Mesh: %s\n", mesh->name);
    printf("Material: %s\n", mesh->material->name);
    for (size_t i = 0; i < mesh->num_vertices; i++) {
        print_vertex(&mesh->vertices[i]);
    }
    for (size_t i = 0; i < mesh->num_indices / 3; i++) {
        printf("Triangle %zu: %d %d %d\n", i, mesh->indices[i * 3],
                mesh->indices[i * 3 + 1], mesh->indices[i * 3 + 2]);
    }
    printf("%zu vertices, %zu indices\n", mesh->num_vertices, mesh->num_indices);
}
/*
void mesh_calculate_normal(MESH *mesh) {
    for (size_t i = 0; i < mesh->num_indices / 3; i++) {
        VERTEX *vtx0 = &mesh->vertices[mesh->indices[i * 3]];
        VERTEX *vtx1 = &mesh->vertices[mesh->indices[i * 3 + 1]];
        VERTEX *vtx2 = &mesh->vertices[mesh->indices[i * 3 + 2]];

        VEC3 v0 = vtx0->position;
        VEC3 v1 = vtx1->position;
        VEC3 v2 = vtx2->position;

        VEC3 edge1 = vec3_sub(v1, v0);
        VEC3 edge2 = vec3_sub(v2, v0);

        VEC3 normal = vec3_normalize(vec3_cross(edge1, edge2));

        vtx0->normal = vec3_add(vtx0->normal, normal);
        vtx1->normal = vec3_add(vtx1->normal, normal);
        vtx2->normal = vec3_add(vtx2->normal, normal);
    }

    for (size_t i = 0; i < mesh->num_vertices; i++) {
        mesh->vertices[i].normal = vec3_normalize(mesh->vertices[i].normal);
    }
}*/

void mesh_init(MESH *mesh) {
    mesh->vbo = s3d_load_vbo(mesh->vertices, mesh->num_vertices * sizeof(VERTEX));
    mesh->ebo = s3d_load_ebo(mesh->indices, mesh->num_indices * sizeof(uint32_t));
    mesh->vao = s3d_bind_vao(mesh->ebo, mesh->vbo, 5, 5);
}

// Return size of the mesh in bytes, for VRAM usage estimation
size_t mesh_size(MESH *mesh) {
    return mesh->num_vertices * sizeof(VERTEX) + mesh->num_indices * sizeof(uint32_t);
}

static bool sphere_within_camera(VEC4 *sphere, CAMERA *camera) {
    VEC4 sph = {sphere->x, sphere->y, sphere->z, 1.f};
    float radius = sphere->w;
    for (int i = 0; i < 6; i++) {
        if (vec4_dot(camera->frustum_planes[i], sph) < -radius) {
            return false;
        }
    }
    return true;
}

void mesh_render_obj(OBJ *obj, CAMERA *camera, RENDERPASS renderpass) {
    SHADER *current_shader = NULL;
    SHADER *new_shader = NULL;
    MATERIAL *current_material = NULL;

    // Setup shader
    if (renderpass == FORWARD_PASS) {
        s3d_update_uniform(&camera->projection_view_matrix, sizeof(MAT4));
        s3d_set_varying_count(2);
        /*shader_use(obj->gbuffer_shader);
        shader_set_mat4(obj->gbuffer_shader, "projection_view_matrix", &camera->projection_view_matrix);
        shader_set_mat4(obj->gbuffer_shader, "view_matrix", &camera->view_matrix);
        shader_set_int(obj->gbuffer_shader, "diffuse_texture", 0);*/
    }

    int count = 0;
    for (size_t i = 0; i < obj->num_meshes; i++) {
        MESH *mesh = &obj->meshes[i];
        // Check if mesh is visiable
        if (!sphere_within_camera(&obj->bounding_spheres[i], camera))
            continue;
        count++;

        // Update shader or material
        if (renderpass == FORWARD_PASS) {
            if (mesh->material != current_material) {
                if (mesh->material->tex_diffuse) {
                    s3d_bind_texture(0, mesh->material->tex_diffuse->id);
                }
                else {
                    s3d_bind_texture(0, 0);
                }
            }
            current_material = mesh->material;
        }

        //printf("Rendering mesh %d\n", i);
        // Render
        s3d_render(mesh->vao);
        //printf("Done.\n");
    }
    //printf("Rendered %d meshes\n", count);
}
