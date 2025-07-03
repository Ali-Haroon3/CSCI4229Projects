/*
 * lighting.h - Advanced Lighting and Shadow System
 */

#ifndef LIGHTING_H
#define LIGHTING_H

#ifdef __APPLE__
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#else
#include <GL/glew.h>
#endif

#define MAX_LIGHTS 16
#define SHADOW_MAP_SIZE 2048

typedef enum {
    LIGHT_POINT,
    LIGHT_DIRECTIONAL,
    LIGHT_SPOT
} LightType;

typedef struct {
    LightType type;
    float position[3];
    float direction[3];
    float color[3];
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float cutoff;
    float outer_cutoff;
    int cast_shadows;
} Light;

typedef struct {
    Light lights[MAX_LIGHTS];
    int num_lights;
    float ambient_color[3];
    float ambient_intensity;
    
    // Shadow mapping
    GLuint shadow_fbo;
    GLuint shadow_map;
    float light_space_matrix[16];
    int shadows_enabled;
    
    // Environment
    GLuint environment_map;
    GLuint irradiance_map;
    GLuint prefilter_map;
    GLuint brdf_lut;
} LightingSystem;

// Function prototypes
LightingSystem* create_lighting_system(void);
void free_lighting_system(LightingSystem* system);

// Light management
int add_light(LightingSystem* system, Light* light);
void remove_light(LightingSystem* system, int index);
void update_light(LightingSystem* system, int index, Light* light);

// Shadow mapping
void init_shadow_mapping(LightingSystem* system);
void begin_shadow_pass(LightingSystem* system, int light_index);
void end_shadow_pass(void);
void update_light_space_matrix(LightingSystem* system, int light_index);

// Shader setup
void set_lighting_uniforms(LightingSystem* system, GLuint shader_program);
void bind_shadow_map(LightingSystem* system, int texture_unit);

// IBL (Image-Based Lighting)
void load_environment_map(LightingSystem* system, const char* hdr_file);
void generate_ibl_maps(LightingSystem* system);

// Utilities
void calculate_light_space_matrix(float* matrix, Light* light, float near, float far);
void get_light_view_matrix(float* matrix, Light* light);
void get_light_projection_matrix(float* matrix, Light* light, float near, float far);

// Matrix math utilities (exposed for other modules)
void matrix_multiply(float* result, const float* m1, const float* m2);
void matrix_look_at(float* matrix, float eye_x, float eye_y, float eye_z,
                   float center_x, float center_y, float center_z,
                   float up_x, float up_y, float up_z);
void matrix_ortho(float* matrix, float left, float right, float bottom, float top, float near, float far);
void matrix_perspective(float* matrix, float fovy, float aspect, float near, float far);
void matrix_rotate_x(float* matrix, float angle);
void matrix_identity(float* matrix);
void matrix_rotate_y(float* matrix, float angle);
void matrix_translate(float* matrix, float x, float y, float z);
#endif // LIGHTING_H
