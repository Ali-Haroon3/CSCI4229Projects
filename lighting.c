/*
 * lighting.c - Advanced Lighting and Shadow System Implementation
 */

#include "lighting.h"
#include "shaders.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// Matrix math helpers
void matrix_multiply(float* result, const float* m1, const float* m2) {
    float temp[16];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            temp[i * 4 + j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                temp[i * 4 + j] += m1[i * 4 + k] * m2[k * 4 + j];
            }
        }
    }
    memcpy(result, temp, 16 * sizeof(float));
}

void matrix_look_at(float* matrix, float eye_x, float eye_y, float eye_z,
                   float center_x, float center_y, float center_z,
                   float up_x, float up_y, float up_z) {
    float f[3] = {center_x - eye_x, center_y - eye_y, center_z - eye_z};
    float f_len = sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    f[0] /= f_len; f[1] /= f_len; f[2] /= f_len;
    
    float s[3] = {
        f[1] * up_z - f[2] * up_y,
        f[2] * up_x - f[0] * up_z,
        f[0] * up_y - f[1] * up_x
    };
    float s_len = sqrt(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
    s[0] /= s_len; s[1] /= s_len; s[2] /= s_len;
    
    float u[3] = {
        s[1] * f[2] - s[2] * f[1],
        s[2] * f[0] - s[0] * f[2],
        s[0] * f[1] - s[1] * f[0]
    };
    
    matrix[0] = s[0]; matrix[4] = s[1]; matrix[8] = s[2]; matrix[12] = -(s[0] * eye_x + s[1] * eye_y + s[2] * eye_z);
    matrix[1] = u[0]; matrix[5] = u[1]; matrix[9] = u[2]; matrix[13] = -(u[0] * eye_x + u[1] * eye_y + u[2] * eye_z);
    matrix[2] = -f[0]; matrix[6] = -f[1]; matrix[10] = -f[2]; matrix[14] = f[0] * eye_x + f[1] * eye_y + f[2] * eye_z;
    matrix[3] = 0.0f; matrix[7] = 0.0f; matrix[11] = 0.0f; matrix[15] = 1.0f;
}

void matrix_ortho(float* matrix, float left, float right, float bottom, float top, float near, float far) {
    memset(matrix, 0, 16 * sizeof(float));
    matrix[0] = 2.0f / (right - left);
    matrix[5] = 2.0f / (top - bottom);
    matrix[10] = -2.0f / (far - near);
    matrix[12] = -(right + left) / (right - left);
    matrix[13] = -(top + bottom) / (top - bottom);
    matrix[14] = -(far + near) / (far - near);
    matrix[15] = 1.0f;
}

void matrix_perspective(float* matrix, float fovy, float aspect, float near, float far) {
    float f = 1.0f / tan(fovy * 0.5f);
    memset(matrix, 0, 16 * sizeof(float));
    matrix[0] = f / aspect;
    matrix[5] = f;
    matrix[10] = (far + near) / (near - far);
    matrix[11] = -1.0f;
    matrix[14] = (2.0f * far * near) / (near - far);
}

// Lighting system creation
LightingSystem* create_lighting_system(void) {
    LightingSystem* system = (LightingSystem*)calloc(1, sizeof(LightingSystem));
    
    // Default ambient
    system->ambient_color[0] = 0.1f;
    system->ambient_color[1] = 0.1f;
    system->ambient_color[2] = 0.15f;
    system->ambient_intensity = 1.0f;
    
    // Initialize shadow mapping
    init_shadow_mapping(system);
    
    return system;
}

void free_lighting_system(LightingSystem* system) {
    if (system) {
        glDeleteFramebuffers(1, &system->shadow_fbo);
        glDeleteTextures(1, &system->shadow_map);
        glDeleteTextures(1, &system->environment_map);
        glDeleteTextures(1, &system->irradiance_map);
        glDeleteTextures(1, &system->prefilter_map);
        glDeleteTextures(1, &system->brdf_lut);
        free(system);
    }
}

// Light management
int add_light(LightingSystem* system, Light* light) {
    if (system->num_lights >= MAX_LIGHTS) {
        return -1;
    }
    
    system->lights[system->num_lights] = *light;
    return system->num_lights++;
}

void remove_light(LightingSystem* system, int index) {
    if (index >= 0 && index < system->num_lights) {
        for (int i = index; i < system->num_lights - 1; i++) {
            system->lights[i] = system->lights[i + 1];
        }
        system->num_lights--;
    }
}

void update_light(LightingSystem* system, int index, Light* light) {
    if (index >= 0 && index < system->num_lights) {
        system->lights[index] = *light;
    }
}

// Shadow mapping
void init_shadow_mapping(LightingSystem* system) {
    // Create framebuffer
    glGenFramebuffers(1, &system->shadow_fbo);
    
    // Create depth texture
    glGenTextures(1, &system->shadow_map);
    glBindTexture(GL_TEXTURE_2D, system->shadow_map);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F,
                 SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    // Attach depth texture to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, system->shadow_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, system->shadow_map, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Shadow framebuffer not complete!\n");
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    system->shadows_enabled = 1;
}

void begin_shadow_pass(LightingSystem* system, int light_index) {
    if (light_index < 0 || light_index >= system->num_lights) return;
    
    // Update light space matrix
    update_light_space_matrix(system, light_index);
    
    // Bind shadow framebuffer
    glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, system->shadow_fbo);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Set up shader
    use_shader(SHADER_SHADOW_MAP);
    set_uniform_mat4(shader_programs[SHADER_SHADOW_MAP].program,
                     "lightSpaceMatrix", system->light_space_matrix);
}

void end_shadow_pass(void) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void update_light_space_matrix(LightingSystem* system, int light_index) {
    if (light_index < 0 || light_index >= system->num_lights) return;
    
    Light* light = &system->lights[light_index];
    calculate_light_space_matrix(system->light_space_matrix, light, 0.1f, 100.0f);
}

void calculate_light_space_matrix(float* matrix, Light* light, float near, float far) {
    float view[16], projection[16];
    
    get_light_view_matrix(view, light);
    get_light_projection_matrix(projection, light, near, far);
    
    matrix_multiply(matrix, projection, view);
}

void get_light_view_matrix(float* matrix, Light* light) {
    if (light->type == LIGHT_DIRECTIONAL) {
        // For directional lights, place the "eye" far away in the opposite direction
        float eye[3] = {
            -light->direction[0] * 50.0f,
            -light->direction[1] * 50.0f,
            -light->direction[2] * 50.0f
        };
        matrix_look_at(matrix, eye[0], eye[1], eye[2], 0, 0, 0, 0, 1, 0);
    } else {
        // For point/spot lights, look from light position towards a target
        float target[3] = {
            light->position[0] + light->direction[0],
            light->position[1] + light->direction[1],
            light->position[2] + light->direction[2]
        };
        matrix_look_at(matrix, light->position[0], light->position[1], light->position[2],
                      target[0], target[1], target[2], 0, 1, 0);
    }
}

void get_light_projection_matrix(float* matrix, Light* light, float near, float far) {
    if (light->type == LIGHT_DIRECTIONAL) {
        // Orthographic projection for directional lights
        float size = 20.0f;
        matrix_ortho(matrix, -size, size, -size, size, near, far);
    } else if (light->type == LIGHT_SPOT) {
        // Perspective projection for spot lights
        float fov = 2.0f * acos(light->cutoff);
        matrix_perspective(matrix, fov, 1.0f, near, far);
    } else {
        // Perspective projection for point lights (would need cube map)
        matrix_perspective(matrix, M_PI / 2.0f, 1.0f, near, far);
    }
}

// Shader setup
void set_lighting_uniforms(LightingSystem* system, GLuint shader_program) {
    glUseProgram(shader_program);
    
    // Set number of lights
    set_uniform_int(shader_program, "numLights", system->num_lights);
    
    // Set each light
    for (int i = 0; i < system->num_lights && i < MAX_LIGHTS; i++) {
        char uniform_name[64];
        Light* light = &system->lights[i];
        
        sprintf(uniform_name, "lightPositions[%d]", i);
        glUniform3fv(glGetUniformLocation(shader_program, uniform_name), 1, light->position);
        
        sprintf(uniform_name, "lightColors[%d]", i);
        glUniform3fv(glGetUniformLocation(shader_program, uniform_name), 1, light->color);
        
        sprintf(uniform_name, "lightIntensities[%d]", i);
        glUniform1f(glGetUniformLocation(shader_program, uniform_name), light->intensity);
    }
    
    // Set ambient
    set_uniform_vec3(shader_program, "ambientColor",
                     system->ambient_color[0] * system->ambient_intensity,
                     system->ambient_color[1] * system->ambient_intensity,
                     system->ambient_color[2] * system->ambient_intensity);
    
    // Set shadow uniforms
    set_uniform_int(shader_program, "shadowsEnabled", system->shadows_enabled);
    set_uniform_mat4(shader_program, "lightSpaceMatrix", system->light_space_matrix);
}

void bind_shadow_map(LightingSystem* system, int texture_unit) {
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(GL_TEXTURE_2D, system->shadow_map);
}
void matrix_rotate_x(float* matrix, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    float r[16];
    matrix_identity(r);
    r[5] = c; r[6] = -s;
    r[9] = s; r[10] = c;
    
    float result[16];
    memcpy(result, matrix, 16 * sizeof(float));
    matrix_multiply(matrix, result, r);
}
void matrix_identity(float* m) {
    memset(m, 0, 16 * sizeof(float));
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

void matrix_rotate_y(float* matrix, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    float r[16];
    matrix_identity(r);
    r[0] = c; r[2] = s;
    r[8] = -s; r[10] = c;
    
    float result[16];
    memcpy(result, matrix, 16 * sizeof(float));
    matrix_multiply(matrix, result, r);
}

void matrix_translate(float* matrix, float x, float y, float z) {
    float t[16];
    matrix_identity(t);
    t[12] = x; t[13] = y; t[14] = z;
    
    float result[16];
    memcpy(result, matrix, 16 * sizeof(float));
    matrix_multiply(matrix, result, t);
}
