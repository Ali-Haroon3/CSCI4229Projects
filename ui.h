/*
 * ui.h - User Interface and HUD System
 */

#ifndef UI_H
#define UI_H

#ifdef __APPLE__
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#else
#include <GL/glew.h>
#endif

// UI Elements
typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint texture;
    GLuint shader_program;
    int visible;
} UIElement;

typedef struct {
    UIElement controls_panel;
    UIElement hotbar;
    UIElement crosshair;
    UIElement gem_counter;
    
    // Text rendering
    GLuint font_texture;
    GLuint text_shader;
    GLuint text_vao;
    GLuint text_vbo;
    
    // Hotbar state
    int selected_slot;
    int gem_counts[10];  // 10 hotbar slots
    int total_gems_collected;
} UISystem;

// Function prototypes
UISystem* create_ui_system(void);
void free_ui_system(UISystem* ui);
void init_ui_shaders(void);
void render_ui(UISystem* ui, int window_width, int window_height);
void render_text(const char* text, float x, float y, float scale);
void render_controls_overlay(int show);
void update_hotbar(UISystem* ui, int slot, int count);
void select_hotbar_slot(UISystem* ui, int slot);

// UI shader sources
extern const char* ui_vertex_shader;
extern const char* ui_fragment_shader;
extern const char* text_vertex_shader;
extern const char* text_fragment_shader;

#endif // UI_H
