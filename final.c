/*
 * final.c - Cave Dweller FINAL PROJECT
 * Advanced cave rendering with tessellation shaders, PBR lighting, and shadows
 *
 * Controls:
 * - WASD: Move camera
 * - Mouse: Look around
 * - Space/Shift: Move up/down
 * - R: Regenerate cave
 * - T: Toggle tessellation level
 * - L: Toggle lighting mode
 * - F: Toggle fog
 * - P: Toggle wireframe
 * - ESC: Exit
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "shaders.h"
#include "cave.h"
#include "lighting.h"
#include "ui.h"

#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#endif

// Window settings
int window_width = 1920;
int window_height = 1080;
double aspect_ratio = 16.0/9.0;
double near_plane = 0.1;
double far_plane = 200.0;

// Camera
typedef struct {
    float position[3];
    float rotation[2];  // yaw, pitch
    float velocity[3];
    float speed;
    float sensitivity;
} Camera;

Camera camera = {
    {0.0f, 2.0f, 5.0f},
    {0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    5.0f,
    0.002f
};

// Mouse state
int mouse_last_x = 0;
int mouse_last_y = 0;
int mouse_captured = 0;

// Keyboard state
int keys[256] = {0};
int shift_pressed = 0;

// Scene objects
Cave* cave = NULL;
CaveMesh* cave_mesh = NULL;
Crystal* crystals = NULL;
int crystal_count = 100;
Gem* gems = NULL;
int gem_count = 200;
LightingSystem* lighting = NULL;
UISystem* ui = NULL;

// Render settings
int wireframe = 0;
int show_fps = 1;
int show_controls = 0;
float tessellation_level = 32.0f;
float time_value = 0.0f;
int fog_enabled = 1;
CaveViewMode view_mode = CAVE_INTERIOR;

// Performance tracking
int frame_count = 0;
float fps = 0.0f;
int last_time = 0;

// Initialize OpenGL
void init_opengl() {
#ifndef __APPLE__
    // Initialize GLEW on non-Apple platforms
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "GLEW initialization failed: %s\n", glewGetErrorString(err));
        exit(1);
    }
    
    // Check for required OpenGL version
    if (!GLEW_VERSION_4_1) {
        fprintf(stderr, "OpenGL 4.1 not supported!\n");
        exit(1);
    }
#else
    // On macOS, check OpenGL version directly
    const char* version = (const char*)glGetString(GL_VERSION);
    printf("OpenGL Version: %s\n", version);
#endif
    
    // OpenGL settings
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Clear color
    glClearColor(0.02f, 0.02f, 0.03f, 1.0f);
    
    // Initialize shaders
    init_shaders();
}

// Initialize scene
void init_scene() {
    // Create cave
    printf("Generating cave...\n");
    cave = create_cave(CAVE_WIDTH, CAVE_HEIGHT, CAVE_DEPTH);
    generate_cave_3d(cave);
    
    printf("Creating cave mesh...\n");
    cave_mesh = create_cave_mesh(cave);
    
    printf("Generating crystals...\n");
    crystals = generate_crystals(cave, crystal_count);
    
    printf("Generating gems...\n");
    gems = generate_gems(cave, gem_count);
    
    // Initialize UI
    printf("Setting up UI...\n");
    ui = create_ui_system();
    
    // Initialize lighting
    printf("Setting up lighting...\n");
    lighting = create_lighting_system();
    
    // Increase ambient lighting for better visibility
    lighting->ambient_color[0] = 0.3f;  // Increased from 0.1f
    lighting->ambient_color[1] = 0.3f;  // Increased from 0.1f
    lighting->ambient_color[2] = 0.4f;  // Increased from 0.15f
    lighting->ambient_intensity = 2.0f; // Increased from 1.0f
    
    // Add main directional light (brighter)
    Light sun = {
        .type = LIGHT_DIRECTIONAL,
        .direction = {-0.3f, -0.8f, -0.5f},
        .color = {1.0f, 0.95f, 0.8f},
        .intensity = 3.0f,  // Increased from 2.0f
        .cast_shadows = 1
    };
    add_light(lighting, &sun);
    
    // Add more point lights for better cave illumination
    for (int i = 0; i < 8; i++) {  // Increased from 5 to 8 lights
        Light point = {
            .type = LIGHT_POINT,
            .position = {
                (rand() % 20 - 10) * 0.5f,
                2.0f + (rand() % 30) * 0.1f,  // Higher placement
                (rand() % 20 - 10) * 0.5f
            },
            .color = {
                0.7f + (rand() % 30) / 100.0f,  // Brighter colors
                0.6f + (rand() % 40) / 100.0f,
                0.9f + (rand() % 10) / 100.0f
            },
            .intensity = 8.0f + (rand() % 40) / 10.0f,  // Much brighter
            .constant = 1.0f,
            .linear = 0.05f,     // Reduced attenuation for more spread
            .quadratic = 0.02f,  // Reduced attenuation
            .cast_shadows = 0
        };
        add_light(lighting, &point);
    }
    
    // Add a player light that follows the camera
    Light player_light = {
        .type = LIGHT_POINT,
        .position = {0.0f, 0.0f, 0.0f},  // Will be updated in render loop
        .color = {1.0f, 1.0f, 0.9f},
        .intensity = 6.0f,
        .constant = 1.0f,
        .linear = 0.08f,
        .quadratic = 0.03f,
        .cast_shadows = 0
    };
    add_light(lighting, &player_light);
    
    // Set spawn point inside cave
    find_spawn_point(cave, &camera.position[0], &camera.position[1], &camera.position[2]);
    
    printf("Scene initialized!\n");
}

// Collision detection helper
int check_collision(Cave* cave, float x, float y, float z, float radius) {
    // Convert world coordinates to cave coordinates
    int cx = (int)((x + 5.0f) / 10.0f * cave->width);
    int cy = (int)((y + 5.0f) / 10.0f * cave->height);
    int cz = (int)((z + 5.0f) / 10.0f * cave->depth);
    
    // Check bounds
    if (cx < 0 || cx >= cave->width ||
        cy < 0 || cy >= cave->height ||
        cz < 0 || cz >= cave->depth) {
        return 1; // Collision with boundaries
    }
    
    // Check a small area around the player position for walls
    int check_radius = (int)(radius * cave->width / 10.0f) + 1;
    
    for (int dz = -check_radius; dz <= check_radius; dz++) {
        for (int dy = -check_radius; dy <= check_radius; dy++) {
            for (int dx = -check_radius; dx <= check_radius; dx++) {
                int test_x = cx + dx;
                int test_y = cy + dy;
                int test_z = cz + dz;
                
                if (test_x >= 0 && test_x < cave->width &&
                    test_y >= 0 && test_y < cave->height &&
                    test_z >= 0 && test_z < cave->depth) {
                    
                    if (cave->map[test_z][test_y][test_x] == 1) {
                        // Calculate distance to this wall block
                        float wall_world_x = (float)test_x / cave->width * 10.0f - 5.0f;
                        float wall_world_y = (float)test_y / cave->height * 10.0f - 5.0f;
                        float wall_world_z = (float)test_z / cave->depth * 10.0f - 5.0f;
                        
                        float dist = sqrt(pow(x - wall_world_x, 2) +
                                        pow(y - wall_world_y, 2) +
                                        pow(z - wall_world_z, 2));
                        
                        if (dist < radius) {
                            return 1; // Collision detected
                        }
                    }
                }
            }
        }
    }
    
    return 0; // No collision
}

// Update camera
void update_camera(float dt) {
    // Calculate new position based on velocity
    float new_pos[3] = {
        camera.position[0] + camera.velocity[0] * dt,
        camera.position[1] + camera.velocity[1] * dt,
        camera.position[2] + camera.velocity[2] * dt
    };
    
    // Check collision for new position
    float collision_radius = 0.3f; // Player collision radius
    
    // Check X movement
    if (!check_collision(cave, new_pos[0], camera.position[1], camera.position[2], collision_radius)) {
        camera.position[0] = new_pos[0];
    }
    
    // Check Y movement
    if (!check_collision(cave, camera.position[0], new_pos[1], camera.position[2], collision_radius)) {
        camera.position[1] = new_pos[1];
    }
    
    // Check Z movement
    if (!check_collision(cave, camera.position[0], camera.position[1], new_pos[2], collision_radius)) {
        camera.position[2] = new_pos[2];
    }
    
    // Update velocity based on input
    float forward[3] = {
        sin(camera.rotation[0]) * cos(camera.rotation[1]),
        -sin(camera.rotation[1]),
        -cos(camera.rotation[0]) * cos(camera.rotation[1])
    };
    
    float right[3] = {
        cos(camera.rotation[0]),
        0,
        sin(camera.rotation[0])
    };
    
    camera.velocity[0] = camera.velocity[1] = camera.velocity[2] = 0;
    
    if (keys['w'] || keys['W']) {
        camera.velocity[0] += forward[0] * camera.speed;
        camera.velocity[1] += forward[1] * camera.speed;
        camera.velocity[2] += forward[2] * camera.speed;
    }
    if (keys['s'] || keys['S']) {
        camera.velocity[0] -= forward[0] * camera.speed;
        camera.velocity[1] -= forward[1] * camera.speed;
        camera.velocity[2] -= forward[2] * camera.speed;
    }
    if (keys['a'] || keys['A']) {
        camera.velocity[0] -= right[0] * camera.speed;
        camera.velocity[2] -= right[2] * camera.speed;
    }
    if (keys['d'] || keys['D']) {
        camera.velocity[0] += right[0] * camera.speed;
        camera.velocity[2] += right[2] * camera.speed;
    }
    if (keys[' ']) {
        camera.velocity[1] += camera.speed;
    }
    if (shift_pressed) {
        camera.velocity[1] -= camera.speed;
    }
    
    // Update player light position to follow camera
    if (lighting && lighting->num_lights > 1) {
        // Assuming the last light is the player light
        int player_light_idx = lighting->num_lights - 1;
        lighting->lights[player_light_idx].position[0] = camera.position[0];
        lighting->lights[player_light_idx].position[1] = camera.position[1] + 0.2f; // Slightly above camera
        lighting->lights[player_light_idx].position[2] = camera.position[2];
    }
}

// Get view matrix
void get_view_matrix(float* matrix) {
    matrix_identity(matrix);
    matrix_rotate_x(matrix, -camera.rotation[1]);
    matrix_rotate_y(matrix, -camera.rotation[0]);
    matrix_translate(matrix, -camera.position[0], -camera.position[1], -camera.position[2]);
}

// Get projection matrix
void get_projection_matrix(float* matrix) {
    matrix_perspective(matrix, M_PI / 4.0f, aspect_ratio, near_plane, far_plane);
}

// Render shadow pass
void render_shadow_pass() {
    // Find main shadow-casting light
    int shadow_light = -1;
    for (int i = 0; i < lighting->num_lights; i++) {
        if (lighting->lights[i].cast_shadows) {
            shadow_light = i;
            break;
        }
    }
    
    if (shadow_light >= 0) {
        begin_shadow_pass(lighting, shadow_light);
        
        // Render cave geometry
        float model[16];
        matrix_identity(model);
        set_uniform_mat4(shader_programs[SHADER_SHADOW_MAP].program, "model", model);
        
        // Note: render_cave_mesh doesn't exist yet, we need to use the tessellation version
        // For shadow pass, we can render with simpler geometry
        glBindVertexArray(cave_mesh->vao);
        glDrawElements(GL_TRIANGLES, cave_mesh->index_count * 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        end_shadow_pass();
    }
}

// Main render function
void render_scene() {
    float view[16], projection[16], model[16];
    
    get_view_matrix(view);
    get_projection_matrix(projection);
    
    if (view_mode == CAVE_EXTERIOR) {
        // Render cave exterior with tessellation
        use_shader(SHADER_TESSELLATION);
        
        matrix_identity(model);
        set_uniform_mat4(shader_programs[SHADER_TESSELLATION].program, "model", model);
        set_uniform_mat4(shader_programs[SHADER_TESSELLATION].program, "view", view);
        set_uniform_mat4(shader_programs[SHADER_TESSELLATION].program, "projection", projection);
        set_uniform_vec3(shader_programs[SHADER_TESSELLATION].program, "viewPos",
                         camera.position[0], camera.position[1], camera.position[2]);
        set_uniform_float(shader_programs[SHADER_TESSELLATION].program, "time", time_value);
        
        // Set lighting
        set_lighting_uniforms(lighting, shader_programs[SHADER_TESSELLATION].program);
        
        // Bind shadow map
        bind_shadow_map(lighting, 6);
        set_uniform_int(shader_programs[SHADER_TESSELLATION].program, "shadowMap", 6);
        
        // Set fog
        set_uniform_vec3(shader_programs[SHADER_TESSELLATION].program, "fogColor", 0.02f, 0.02f, 0.03f);
        set_uniform_float(shader_programs[SHADER_TESSELLATION].program, "fogDensity", fog_enabled ? 0.05f : 0.0f);
        
        render_cave_with_tessellation(cave_mesh);
    } else {
        // Render cave interior
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(60.0, aspect_ratio, 0.1, 100.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        // Apply camera transformation
        glRotatef(-camera.rotation[1] * 180.0f / M_PI, 1, 0, 0);
        glRotatef(-camera.rotation[0] * 180.0f / M_PI, 0, 1, 0);
        glTranslatef(-camera.position[0], -camera.position[1], -camera.position[2]);
        
        render_cave_interior(cave, camera.position[0], camera.position[1], camera.position[2]);
    }
    
    // Render gems
    if (gems && gem_count > 0) {
        use_shader(SHADER_CRYSTAL);
        set_uniform_mat4(shader_programs[SHADER_CRYSTAL].program, "view", view);
        set_uniform_mat4(shader_programs[SHADER_CRYSTAL].program, "projection", projection);
        set_uniform_vec3(shader_programs[SHADER_CRYSTAL].program, "viewPos",
                         camera.position[0], camera.position[1], camera.position[2]);
        set_uniform_float(shader_programs[SHADER_CRYSTAL].program, "time", time_value);
        
        render_gems(gems, gem_count, time_value);
    }
    
    // Render crystals
    if (crystals && crystal_count > 0 && view_mode == CAVE_EXTERIOR) {
        use_shader(SHADER_CRYSTAL);
        set_uniform_mat4(shader_programs[SHADER_CRYSTAL].program, "view", view);
        set_uniform_mat4(shader_programs[SHADER_CRYSTAL].program, "projection", projection);
        set_uniform_vec3(shader_programs[SHADER_CRYSTAL].program, "viewPos",
                         camera.position[0], camera.position[1], camera.position[2]);
        set_uniform_float(shader_programs[SHADER_CRYSTAL].program, "time", time_value);
        
        render_crystals(crystals, crystal_count);
    }
}

// Display callback
void display() {
    // Update time
    int current_time = glutGet(GLUT_ELAPSED_TIME);
    float dt = (current_time - last_time) / 1000.0f;
    last_time = current_time;
    time_value += dt;
    
    // Update FPS
    frame_count++;
    if (frame_count >= 60) {
        fps = frame_count / (time_value - (int)(time_value - 1));
        frame_count = 0;
    }
    
    // Update camera
    update_camera(dt);
    
    // Check for gem collection
    if (keys['e'] || keys['E']) {
        int gem_type = collect_gem(gems, gem_count, camera.position[0], camera.position[1], camera.position[2], 0.5f);
        if (gem_type >= 0) {
            ui->gem_counts[gem_type]++;
            ui->total_gems_collected++;
            update_hotbar(ui, gem_type, ui->gem_counts[gem_type]);
            printf("Collected gem type %d! Total: %d\n", gem_type, ui->total_gems_collected);
        }
    }
    
    // Shadow pass
    if (view_mode == CAVE_EXTERIOR) {
        render_shadow_pass();
    }
    
    // Reset viewport
    glViewport(0, 0, window_width, window_height);
    
    // Main pass
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    render_scene();
    
    // Render UI
    render_ui(ui, window_width, window_height);
    
    // Render controls overlay if enabled
    render_controls_overlay(show_controls);
    
    // Display FPS
    if (show_fps) {
        // Note: Text rendering in core profile requires more setup
        // For now, just print to console
        if (frame_count % 60 == 0) {
            printf("FPS: %.1f\n", fps);
        }
    }
    
    glutSwapBuffers();
}

// Keyboard callbacks
void keyboard(unsigned char key, int x, int y) {
    keys[key] = 1;
    
    // Check for shift
    int modifiers = glutGetModifiers();
    shift_pressed = (modifiers & GLUT_ACTIVE_SHIFT) ? 1 : 0;
    
    switch (key) {
        case 27:  // ESC
            cleanup_shaders();
            free_cave_mesh(cave_mesh);
            free_cave(cave);
            free(crystals);
            free(gems);
            free_lighting_system(lighting);
            free_ui_system(ui);
            exit(0);
            break;
        case 'r':
        case 'R':
            // Regenerate cave
            free_cave(cave);
            free_cave_mesh(cave_mesh);
            free(crystals);
            free(gems);
            cave = create_cave(CAVE_WIDTH, CAVE_HEIGHT, CAVE_DEPTH);
            generate_cave_3d(cave);
            cave_mesh = create_cave_mesh(cave);
            crystals = generate_crystals(cave, crystal_count);
            gems = generate_gems(cave, gem_count);
            find_spawn_point(cave, &camera.position[0], &camera.position[1], &camera.position[2]);
            break;
        case 't':
        case 'T':
            tessellation_level = (tessellation_level >= 64.0f) ? 4.0f : tessellation_level * 2.0f;
            printf("Tessellation level: %.0f\n", tessellation_level);
            break;
        case 'p':
        case 'P':
            wireframe = !wireframe;
            break;
        case 'f':
        case 'F':
            fog_enabled = !fog_enabled;
            break;
        case 'l':
        case 'L':
            lighting->shadows_enabled = !lighting->shadows_enabled;
            break;
        case 'h':
        case 'H':
            show_controls = !show_controls;
            break;
        case 'i':
        case 'I':
            view_mode = (view_mode == CAVE_INTERIOR) ? CAVE_EXTERIOR : CAVE_INTERIOR;
            printf("View mode: %s\n", view_mode == CAVE_INTERIOR ? "Interior" : "Exterior");
            break;
        case 'q':
        case 'Q':
            // Drop gem from selected slot
            if (ui->gem_counts[ui->selected_slot] > 0) {
                ui->gem_counts[ui->selected_slot]--;
                update_hotbar(ui, ui->selected_slot, ui->gem_counts[ui->selected_slot]);
            }
            break;
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9': case '0':
            {
                int slot = (key == '0') ? 9 : (key - '1');
                select_hotbar_slot(ui, slot);
            }
            break;
    }
}

void keyboard_up(unsigned char key, int x, int y) {
    keys[key] = 0;
    
    // Update shift state
    int modifiers = glutGetModifiers();
    shift_pressed = (modifiers & GLUT_ACTIVE_SHIFT) ? 1 : 0;
}

// Mouse callbacks
void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            mouse_captured = 1;
            mouse_last_x = x;
            mouse_last_y = y;
            glutSetCursor(GLUT_CURSOR_NONE);
        } else {
            mouse_captured = 0;
            glutSetCursor(GLUT_CURSOR_INHERIT);
        }
    }
}

void motion(int x, int y) {
    if (mouse_captured) {
        int dx = x - mouse_last_x;
        int dy = y - mouse_last_y;
        
        camera.rotation[0] += dx * camera.sensitivity;
        camera.rotation[1] += dy * camera.sensitivity;
        
        // Clamp pitch
        if (camera.rotation[1] > M_PI / 2 - 0.01f) camera.rotation[1] = M_PI / 2 - 0.01f;
        if (camera.rotation[1] < -M_PI / 2 + 0.01f) camera.rotation[1] = -M_PI / 2 + 0.01f;
        
        // Wrap yaw
        while (camera.rotation[0] > M_PI) camera.rotation[0] -= 2 * M_PI;
        while (camera.rotation[0] < -M_PI) camera.rotation[0] += 2 * M_PI;
        
        // Reset mouse to center
        if (abs(dx) > 1 || abs(dy) > 1) {
            mouse_last_x = window_width / 2;
            mouse_last_y = window_height / 2;
            glutWarpPointer(mouse_last_x, mouse_last_y);
        } else {
            mouse_last_x = x;
            mouse_last_y = y;
        }
    }
}

// Reshape callback
void reshape(int width, int height) {
    window_width = width;
    window_height = height;
    aspect_ratio = (double)width / height;
    glViewport(0, 0, width, height);
}

// Idle callback
void idle() {
    glutPostRedisplay();
}

// Main function
int main(int argc, char** argv) {
    // Initialize GLUT
    glutInit(&argc, argv);
    
#ifdef __APPLE__
    // Request OpenGL 4.1 Core Profile for macOS
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
#else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
#endif
    
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("Cave Dweller - Advanced Tessellation Renderer");
    
    // Initialize OpenGL
    init_opengl();
    
    // Initialize scene
    init_scene();
    
    // Set callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboard_up);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(motion);
    glutIdleFunc(idle);
    
    // Start
    printf("\nControls:\n");
    printf("- WASD: Move\n");
    printf("- Mouse: Look around\n");
    printf("- Space/Shift: Up/Down\n");
    printf("- E: Collect gem\n");
    printf("- Q: Drop gem\n");
    printf("- 1-9,0: Select hotbar slot\n");
    printf("- H: Toggle help overlay\n");
    printf("- I: Toggle interior/exterior view\n");
    printf("- R: Regenerate cave\n");
    printf("- T: Cycle tessellation level\n");
    printf("- P: Toggle wireframe\n");
    printf("- F: Toggle fog\n");
    printf("- L: Toggle shadows\n");
    printf("- ESC: Exit\n\n");
    
    last_time = glutGet(GLUT_ELAPSED_TIME);
    
    glutMainLoop();
    
    return 0;
}
