/*
 * ui.c - User Interface Implementation
 */

#include "ui.h"
#include "shaders.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

// Simple UI vertex shader
const char* ui_vertex_shader =
"#version 410 core\n"
"layout(location = 0) in vec2 position;\n"
"layout(location = 1) in vec2 texCoord;\n"
"out vec2 TexCoord;\n"
"uniform mat4 projection;\n"
"void main() {\n"
"    gl_Position = projection * vec4(position, 0.0, 1.0);\n"
"    TexCoord = texCoord;\n"
"}\n";

// Simple UI fragment shader
const char* ui_fragment_shader =
"#version 410 core\n"
"in vec2 TexCoord;\n"
"out vec4 FragColor;\n"
"uniform sampler2D uiTexture;\n"
"uniform vec4 color;\n"
"void main() {\n"
"    FragColor = texture(uiTexture, TexCoord) * color;\n"
"}\n";

// Text rendering vertex shader
const char* text_vertex_shader =
"#version 410 core\n"
"layout(location = 0) in vec4 vertex;\n"
"out vec2 TexCoords;\n"
"uniform mat4 projection;\n"
"void main() {\n"
"    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
"    TexCoords = vertex.zw;\n"
"}\n";

// Text rendering fragment shader
const char* text_fragment_shader =
"#version 410 core\n"
"in vec2 TexCoords;\n"
"out vec4 color;\n"
"uniform sampler2D text;\n"
"uniform vec3 textColor;\n"
"void main() {\n"
"    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
"    color = vec4(textColor, 1.0) * sampled;\n"
"}\n";

// ASCII font bitmap (simple 8x8 font)
static unsigned char font_bitmap[128][8] = {
    // Space through DEL - simplified bitmap font
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    // ... (would include full ASCII set in real implementation)
};

UISystem* create_ui_system(void) {
    UISystem* ui = (UISystem*)calloc(1, sizeof(UISystem));
    
    // Initialize UI elements
    init_ui_shaders();
    
    // Create crosshair
    float crosshair_verts[] = {
        // Horizontal line
        -0.02f,  0.0f, 0.0f, 0.5f,
         0.02f,  0.0f, 1.0f, 0.5f,
        // Vertical line
         0.0f, -0.02f, 0.5f, 0.0f,
         0.0f,  0.02f, 0.5f, 1.0f
    };
    
    glGenVertexArrays(1, &ui->crosshair.vao);
    glGenBuffers(1, &ui->crosshair.vbo);
    glBindVertexArray(ui->crosshair.vao);
    glBindBuffer(GL_ARRAY_BUFFER, ui->crosshair.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshair_verts), crosshair_verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    
    // Create simple white texture for UI elements
    unsigned char white[] = {255, 255, 255, 255};
    glGenTextures(1, &ui->crosshair.texture);
    glBindTexture(GL_TEXTURE_2D, ui->crosshair.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    ui->crosshair.visible = 1;
    
    return ui;
}

void free_ui_system(UISystem* ui) {
    if (ui) {
        glDeleteVertexArrays(1, &ui->crosshair.vao);
        glDeleteBuffers(1, &ui->crosshair.vbo);
        glDeleteTextures(1, &ui->crosshair.texture);
        glDeleteTextures(1, &ui->font_texture);
        free(ui);
    }
}

void init_ui_shaders(void) {
    // UI shaders are compiled as part of the main shader system
}

void render_text(const char* text, float x, float y, float scale) {
    // Simplified text rendering - in a real implementation,
    // this would use FreeType or a bitmap font
    glUseProgram(0); // Use fixed function for now
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x, y);
    
    for (const char* c = text; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

void render_controls_overlay(int show) {
    if (!show) return;
    
    // Draw semi-transparent background
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Use immediate mode for simplicity (would use VAO in production)
    glUseProgram(0);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Background panel
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(-0.8f, -0.8f);
    glVertex2f(0.8f, -0.8f);
    glVertex2f(0.8f, 0.8f);
    glVertex2f(-0.8f, 0.8f);
    glEnd();
    
    // Title
    glColor3f(1.0f, 1.0f, 1.0f);
    render_text("CAVE DWELLER - CONTROLS", -0.3f, 0.7f, 1.5f);
    
    // Controls list
    float y_pos = 0.5f;
    const char* controls[] = {
        "MOVEMENT:",
        "  W/A/S/D - Move Forward/Left/Back/Right",
        "  Space - Jump/Fly Up",
        "  Shift - Crouch/Fly Down",
        "  Mouse - Look Around",
        "",
        "ACTIONS:",
        "  E - Collect Gem",
        "  Q - Drop Gem",
        "  1-9 - Select Hotbar Slot",
        "  Mouse Wheel - Scroll Hotbar",
        "",
        "DISPLAY:",
        "  H - Toggle This Help",
        "  F - Toggle Fog",
        "  P - Toggle Wireframe",
        "  T - Change Tessellation Level",
        "  L - Toggle Shadows",
        "  R - Regenerate Cave",
        "  I - Toggle Interior/Exterior View",
        "",
        "ESC - Exit Game",
        NULL
    };
    
    for (int i = 0; controls[i]; i++) {
        render_text(controls[i], -0.7f, y_pos, 1.0f);
        y_pos -= 0.06f;
    }
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void render_ui(UISystem* ui, int window_width, int window_height) {
    // Save state
    GLboolean depth_test = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set up orthographic projection
    glUseProgram(0);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, window_width, window_height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Render crosshair
    if (ui->crosshair.visible) {
        glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        // Horizontal
        glVertex2f(window_width/2 - 10, window_height/2);
        glVertex2f(window_width/2 + 10, window_height/2);
        // Vertical
        glVertex2f(window_width/2, window_height/2 - 10);
        glVertex2f(window_width/2, window_height/2 + 10);
        glEnd();
    }
    
    // Render hotbar
    float hotbar_width = 500;
    float slot_size = 50;
    float hotbar_x = window_width/2 - hotbar_width/2;
    float hotbar_y = window_height - 80;
    
    // Hotbar background
    glColor4f(0.2f, 0.2f, 0.2f, 0.8f);
    for (int i = 0; i < 10; i++) {
        float x = hotbar_x + i * slot_size;
        
        // Highlight selected slot
        if (i == ui->selected_slot) {
            glColor4f(0.8f, 0.8f, 0.2f, 0.9f);
        } else {
            glColor4f(0.2f, 0.2f, 0.2f, 0.8f);
        }
        
        glBegin(GL_QUADS);
        glVertex2f(x, hotbar_y);
        glVertex2f(x + slot_size - 2, hotbar_y);
        glVertex2f(x + slot_size - 2, hotbar_y + slot_size);
        glVertex2f(x, hotbar_y + slot_size);
        glEnd();
        
        // Draw gem count
        if (ui->gem_counts[i] > 0) {
            char count_str[32];
            sprintf(count_str, "%d", ui->gem_counts[i]);
            glColor3f(1.0f, 1.0f, 1.0f);
            glRasterPos2f(x + 5, hotbar_y + slot_size - 5);
            for (char* c = count_str; *c; c++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
            }
        }
        
        // Slot number
        glColor3f(0.8f, 0.8f, 0.8f);
        glRasterPos2f(x + 5, hotbar_y + 15);
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, '1' + i);
    }
    
    // Gem counter
    char gem_text[64];
    sprintf(gem_text, "Gems Collected: %d", ui->total_gems_collected);
    glColor3f(1.0f, 1.0f, 0.0f);
    glRasterPos2f(10, 30);
    for (char* c = gem_text; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
    
    // FPS counter
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(window_width - 100, 30);
    
    // Press H for help
    glColor3f(0.8f, 0.8f, 0.8f);
    glRasterPos2f(10, window_height - 20);
    const char* help_text = "Press H for controls";
    for (const char* c = help_text; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
    
    // Restore state
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    
    if (depth_test) glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void update_hotbar(UISystem* ui, int slot, int count) {
    if (slot >= 0 && slot < 10) {
        ui->gem_counts[slot] = count;
    }
}

void select_hotbar_slot(UISystem* ui, int slot) {
    if (slot >= 0 && slot < 10) {
        ui->selected_slot = slot;
    }
}
