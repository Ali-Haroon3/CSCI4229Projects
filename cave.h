/*
 * cave.h - Cave Generation and Rendering
 */

#ifndef CAVE_H
#define CAVE_H

#ifdef __APPLE__
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#else
#include <GL/glew.h>
#endif

#include <stdlib.h>
#include <math.h>

#define CAVE_WIDTH 100
#define CAVE_HEIGHT 100
#define CAVE_DEPTH 50
#define WALL_THRESHOLD_PERCENTAGE 45
#define SMOOTHING_ITERATIONS 5
#define MIN_CAVE_SIZE 40

// Cave map structure
typedef struct {
    int*** map;  // 3D cave map
    int width;
    int height;
    int depth;
    float* height_map;  // 2D height map for terrain
    float* normal_map;  // Normal map data
} Cave;

// Cave mesh structure for rendering
typedef struct {
    GLuint vao;
    GLuint vbo_vertices;
    GLuint vbo_normals;
    GLuint vbo_texcoords;
    GLuint ebo;
    GLuint height_texture;
    GLuint normal_texture;
    GLuint diffuse_texture;
    GLuint roughness_texture;
    GLuint ao_texture;
    GLuint emissive_texture;
    int index_count;
    int patch_count_x;
    int patch_count_z;
} CaveMesh;

// Crystal structure
typedef struct {
    float x, y, z;
    float size;
    float rotation;
    float color[3];
    float glow_intensity;
} Crystal;

// Gem structure (collectible)
typedef struct {
    float x, y, z;
    float rotation;
    float bob_offset;
    int type;  // 0-9 for different gem types
    int collected;
    float color[3];
    float size;
} Gem;

// Cave interior mode
typedef enum {
    CAVE_EXTERIOR,
    CAVE_INTERIOR
} CaveViewMode;

// Water plane
typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint reflection_fbo;
    GLuint refraction_fbo;
    GLuint reflection_texture;
    GLuint refraction_texture;
    GLuint reflection_depth;
    GLuint refraction_depth;
    GLuint dudv_texture;
    GLuint normal_texture;
    float water_level;
} WaterPlane;

// Function prototypes
Cave* create_cave(int width, int height, int depth);
void free_cave(Cave* cave);
void generate_cave_3d(Cave* cave);
void smooth_cave(Cave* cave);
void generate_height_map(Cave* cave);
void generate_normal_map(Cave* cave);
void carve_cave_interior(Cave* cave);
void find_spawn_point(Cave* cave, float* x, float* y, float* z);

CaveMesh* create_cave_mesh(Cave* cave);
void free_cave_mesh(CaveMesh* mesh);
void update_cave_mesh(CaveMesh* mesh, Cave* cave);
void render_cave_mesh(CaveMesh* mesh);
void render_cave_with_tessellation(CaveMesh* mesh);
void render_cave_interior(Cave* cave, float cam_x, float cam_y, float cam_z);

Crystal* generate_crystals(Cave* cave, int count);
void render_crystals(Crystal* crystals, int count);

Gem* generate_gems(Cave* cave, int count);
void render_gems(Gem* gems, int count, float time);
int collect_gem(Gem* gems, int count, float player_x, float player_y, float player_z, float collect_radius);
void respawn_gem(Gem* gem, Cave* cave);

WaterPlane* create_water_plane(float size, float level);
void free_water_plane(WaterPlane* water);
void begin_water_reflection_pass(WaterPlane* water);
void begin_water_refraction_pass(WaterPlane* water);
void end_water_pass(void);
void render_water(WaterPlane* water);

// Texture loading
GLuint load_texture(const char* filename);
GLuint create_texture_from_data(const float* data, int width, int height, int channels);

// Utility functions
float perlin_noise_3d(float x, float y, float z);
float fractal_noise_3d(float x, float y, float z, int octaves, float persistence);
void calculate_tangent_space(const float* v0, const float* v1, const float* v2,
                            const float* uv0, const float* uv1, const float* uv2,
                            float* tangent, float* bitangent);

#endif // CAVE_H
