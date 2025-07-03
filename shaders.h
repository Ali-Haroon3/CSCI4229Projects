/*
 * shaders.h - Shader Management for Cave Renderer
 * Handles shader compilation, linking, and program management
 */

#ifndef SHADERS_H
#define SHADERS_H

#ifdef __APPLE__
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#else
#include <GL/glew.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Shader types
typedef enum {
    SHADER_TESSELLATION,
    SHADER_SHADOW_MAP,
    SHADER_CRYSTAL,
    SHADER_WATER,
    SHADER_POST_PROCESS,
    SHADER_COUNT
} ShaderType;

// Shader program structure
typedef struct {
    GLuint program;
    GLuint vertex_shader;
    GLuint tess_control_shader;
    GLuint tess_eval_shader;
    GLuint geometry_shader;
    GLuint fragment_shader;
    
    // Common uniform locations
    GLint model_loc;
    GLint view_loc;
    GLint projection_loc;
    GLint mvp_loc;
    GLint normal_matrix_loc;
    GLint light_space_matrix_loc;
    GLint view_pos_loc;
    GLint time_loc;
} ShaderProgram;

// Global shader programs
extern ShaderProgram shader_programs[SHADER_COUNT];

// Function prototypes
int check_shader_compile_status(GLuint shader);
int check_program_link_status(GLuint program);
GLuint compile_shader(const char* source, GLenum type);
GLuint create_shader_program(const char* vertex_source, const char* fragment_source);
GLuint create_tessellation_program(const char* vertex_source, const char* tcs_source,
                                  const char* tes_source, const char* fragment_source);
GLuint create_full_program(const char* vertex_source, const char* tcs_source,
                          const char* tes_source, const char* geometry_source,
                          const char* fragment_source);
void init_shaders(void);
void cleanup_shaders(void);
void use_shader(ShaderType type);
void set_uniform_mat4(GLuint program, const char* name, const float* matrix);
void set_uniform_vec3(GLuint program, const char* name, float x, float y, float z);
void set_uniform_float(GLuint program, const char* name, float value);
void set_uniform_int(GLuint program, const char* name, int value);

// Shader source strings
extern const char* tessellation_vertex_shader;
extern const char* tessellation_tcs_shader;
extern const char* tessellation_tes_shader;
extern const char* tessellation_fragment_shader;
extern const char* shadow_vertex_shader;
extern const char* shadow_fragment_shader;
extern const char* crystal_vertex_shader;
extern const char* crystal_fragment_shader;
extern const char* water_vertex_shader;
extern const char* water_fragment_shader;

#endif // SHADERS_H
