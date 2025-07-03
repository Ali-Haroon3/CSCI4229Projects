/*
 * cave.c - Cave Generation and Rendering Implementation (COMPLETE)
 */

#include "cave.h"
#include "shaders.h"
#include "lighting.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

// Matrix helper functions (only the ones not in lighting.h)
void matrix_scale(float* m, float sx, float sy, float sz) {
    float s[16];
    matrix_identity(s);
    s[0] = sx; s[5] = sy; s[10] = sz;
    
    float result[16];
    memcpy(result, m, 16 * sizeof(float));
    matrix_multiply(m, result, s);
}

// Forward declarations for texture generation functions
GLuint generate_rock_texture(int width, int height);
GLuint generate_roughness_texture(int width, int height);
GLuint generate_ao_texture(int width, int height);
GLuint generate_crystal_emissive_texture(int width, int height);

// Perlin noise implementation
static int p[512];
static int permutation[] = { 151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,65,25,63,
    161,1,216,80,73,209,76,132,187,208,89,18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,250,124,
    123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,
    221,153,101,155,167,43,172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,228,251,34,242,193,238,
    210,144,12,191,179,162,241,81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,
    150,254,138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180 };

static void init_perlin() {
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 256; i++) {
            p[256 + i] = p[i] = permutation[i];
        }
        initialized = 1;
    }
}

static double fade(double t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

static double lerp(double t, double a, double b) {
    return a + t * (b - a);
}

static double grad(int hash, double x, double y, double z) {
    int h = hash & 15;
    double u = h < 8 ? x : y;
    double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float perlin_noise_3d(float x, float y, float z) {
    init_perlin();
    
    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;
    int Z = (int)floor(z) & 255;
    
    x -= floor(x);
    y -= floor(y);
    z -= floor(z);
    
    double u = fade(x);
    double v = fade(y);
    double w = fade(z);
    
    int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
    int B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;
    
    return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z),
                                   grad(p[BA], x - 1, y, z)),
                           lerp(u, grad(p[AB], x, y - 1, z),
                                   grad(p[BB], x - 1, y - 1, z))),
                   lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1),
                                   grad(p[BA + 1], x - 1, y, z - 1)),
                           lerp(u, grad(p[AB + 1], x, y - 1, z - 1),
                                   grad(p[BB + 1], x - 1, y - 1, z - 1))));
}

float fractal_noise_3d(float x, float y, float z, int octaves, float persistence) {
    float total = 0;
    float frequency = 1;
    float amplitude = 1;
    float maxValue = 0;
    
    for (int i = 0; i < octaves; i++) {
        total += perlin_noise_3d(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2;
    }
    
    return total / maxValue;
}

// Cave generation
Cave* create_cave(int width, int height, int depth) {
    Cave* cave = (Cave*)malloc(sizeof(Cave));
    cave->width = width;
    cave->height = height;
    cave->depth = depth;
    
    // Allocate 3D map
    cave->map = (int***)malloc(depth * sizeof(int**));
    for (int z = 0; z < depth; z++) {
        cave->map[z] = (int**)malloc(height * sizeof(int*));
        for (int y = 0; y < height; y++) {
            cave->map[z][y] = (int*)calloc(width, sizeof(int));
        }
    }
    
    // Allocate height map and normal map
    cave->height_map = (float*)calloc(width * height, sizeof(float));
    cave->normal_map = (float*)calloc(width * height * 3, sizeof(float));
    
    return cave;
}

void free_cave(Cave* cave) {
    if (cave) {
        for (int z = 0; z < cave->depth; z++) {
            for (int y = 0; y < cave->height; y++) {
                free(cave->map[z][y]);
            }
            free(cave->map[z]);
        }
        free(cave->map);
        free(cave->height_map);
        free(cave->normal_map);
        free(cave);
    }
}

void generate_cave_3d(Cave* cave) {
    srand(time(NULL));
    
    // Initialize with random noise
    for (int z = 0; z < cave->depth; z++) {
        for (int y = 0; y < cave->height; y++) {
            for (int x = 0; x < cave->width; x++) {
                cave->map[z][y][x] = (rand() % 100 < WALL_THRESHOLD_PERCENTAGE) ? 1 : 0;
            }
        }
    }
    
    // Apply cellular automata rules
    for (int i = 0; i < SMOOTHING_ITERATIONS; i++) {
        smooth_cave(cave);
    }
    
    // Carve out interior space
    carve_cave_interior(cave);
    
    // Generate terrain features
    generate_height_map(cave);
    generate_normal_map(cave);
}

void smooth_cave(Cave* cave) {
    int*** new_map = (int***)malloc(cave->depth * sizeof(int**));
    for (int z = 0; z < cave->depth; z++) {
        new_map[z] = (int**)malloc(cave->height * sizeof(int*));
        for (int y = 0; y < cave->height; y++) {
            new_map[z][y] = (int*)malloc(cave->width * sizeof(int));
        }
    }
    
    for (int z = 1; z < cave->depth - 1; z++) {
        for (int y = 1; y < cave->height - 1; y++) {
            for (int x = 1; x < cave->width - 1; x++) {
                int wall_count = 0;
                
                // Count surrounding walls in 3x3x3 cube
                for (int nz = z - 1; nz <= z + 1; nz++) {
                    for (int ny = y - 1; ny <= y + 1; ny++) {
                        for (int nx = x - 1; nx <= x + 1; nx++) {
                            if (nz != z || ny != y || nx != x) {
                                wall_count += cave->map[nz][ny][nx];
                            }
                        }
                    }
                }
                
                // Apply cellular automata rules
                new_map[z][y][x] = (wall_count > 13) ? 1 : 0;
            }
        }
    }
    
    // Copy back
    for (int z = 1; z < cave->depth - 1; z++) {
        for (int y = 1; y < cave->height - 1; y++) {
            memcpy(cave->map[z][y], new_map[z][y], cave->width * sizeof(int));
        }
    }
    
    // Free temporary map
    for (int z = 0; z < cave->depth; z++) {
        for (int y = 0; y < cave->height; y++) {
            free(new_map[z][y]);
        }
        free(new_map[z]);
    }
    free(new_map);
}

void generate_height_map(Cave* cave) {
    for (int y = 0; y < cave->height; y++) {
        for (int x = 0; x < cave->width; x++) {
            float base_height = 0.0f;
            
            // Sample multiple layers of the cave to determine height
            for (int z = 0; z < cave->depth; z++) {
                if (cave->map[z][y][x] == 0) {  // Empty space
                    base_height += 0.02f;
                }
            }
            
            // Add procedural detail
            float noise = fractal_noise_3d(x * 0.1f, y * 0.1f, 0.0f, 4, 0.5f);
            float detail = fractal_noise_3d(x * 0.5f, y * 0.5f, 0.0f, 2, 0.3f);
            
            cave->height_map[y * cave->width + x] = base_height + noise * 0.3f + detail * 0.1f;
        }
    }
}

void generate_normal_map(Cave* cave) {
    for (int y = 1; y < cave->height - 1; y++) {
        for (int x = 1; x < cave->width - 1; x++) {
            // Calculate normal using Sobel operator
            float h_l = cave->height_map[y * cave->width + (x - 1)];
            float h_r = cave->height_map[y * cave->width + (x + 1)];
            float h_d = cave->height_map[(y - 1) * cave->width + x];
            float h_u = cave->height_map[(y + 1) * cave->width + x];
            
            float dx = (h_r - h_l) * 2.0f;
            float dy = (h_u - h_d) * 2.0f;
            float dz = 1.0f;
            
            // Normalize
            float len = sqrt(dx * dx + dy * dy + dz * dz);
            dx /= len;
            dy /= len;
            dz /= len;
            
            int idx = (y * cave->width + x) * 3;
            cave->normal_map[idx + 0] = dx * 0.5f + 0.5f;
            cave->normal_map[idx + 1] = dy * 0.5f + 0.5f;
            cave->normal_map[idx + 2] = dz * 0.5f + 0.5f;
        }
    }
}

// Carve interior cave system
void carve_cave_interior(Cave* cave) {
    // Create main chamber in center
    int center_x = cave->width / 2;
    int center_y = cave->height / 2;
    int center_z = cave->depth / 2;
    int chamber_radius = 15;
    
    // Carve main chamber
    for (int z = center_z - chamber_radius; z < center_z + chamber_radius; z++) {
        for (int y = center_y - chamber_radius; y < center_y + chamber_radius; y++) {
            for (int x = center_x - chamber_radius; x < center_x + chamber_radius; x++) {
                if (x > 0 && x < cave->width - 1 &&
                    y > 0 && y < cave->height - 1 &&
                    z > 0 && z < cave->depth - 1) {
                    float dist = sqrt(pow(x - center_x, 2) + pow(y - center_y, 2) + pow(z - center_z, 2));
                    if (dist < chamber_radius) {
                        cave->map[z][y][x] = 0;  // Empty space
                    }
                }
            }
        }
    }
    
    // Carve tunnels
    int num_tunnels = 6 + rand() % 4;
    for (int t = 0; t < num_tunnels; t++) {
        int start_x = center_x;
        int start_y = center_y;
        int start_z = center_z;
        
        // Random direction
        float angle_h = (rand() % 360) * M_PI / 180.0f;
        float angle_v = ((rand() % 60) - 30) * M_PI / 180.0f;
        float dx = cos(angle_h) * cos(angle_v);
        float dy = sin(angle_v);
        float dz = sin(angle_h) * cos(angle_v);
        
        // Carve tunnel
        float x = start_x, y = start_y, z = start_z;
        int tunnel_length = 20 + rand() % 30;
        
        for (int i = 0; i < tunnel_length; i++) {
            int ix = (int)x, iy = (int)y, iz = (int)z;
            
            // Carve a sphere at current position
            int radius = 3 + (rand() % 2);
            for (int sz = -radius; sz <= radius; sz++) {
                for (int sy = -radius; sy <= radius; sy++) {
                    for (int sx = -radius; sx <= radius; sx++) {
                        int px = ix + sx;
                        int py = iy + sy;
                        int pz = iz + sz;
                        
                        if (px > 0 && px < cave->width - 1 &&
                            py > 0 && py < cave->height - 1 &&
                            pz > 0 && pz < cave->depth - 1) {
                            float dist = sqrt(sx*sx + sy*sy + sz*sz);
                            if (dist <= radius) {
                                cave->map[pz][py][px] = 0;
                            }
                        }
                    }
                }
            }
            
            // Move along tunnel direction with some randomness
            x += dx * 2.0f + (rand() % 3 - 1) * 0.5f;
            y += dy * 2.0f + (rand() % 3 - 1) * 0.3f;
            z += dz * 2.0f + (rand() % 3 - 1) * 0.5f;
            
            // Slightly adjust direction
            angle_h += (rand() % 40 - 20) * M_PI / 180.0f * 0.1f;
            angle_v += (rand() % 20 - 10) * M_PI / 180.0f * 0.1f;
            dx = cos(angle_h) * cos(angle_v);
            dy = sin(angle_v);
            dz = sin(angle_h) * cos(angle_v);
        }
    }
}

// Find a good spawn point inside the cave
void find_spawn_point(Cave* cave, float* x, float* y, float* z) {
    // Start from center and find an empty space
    int center_x = cave->width / 2;
    int center_y = cave->height / 2;
    int center_z = cave->depth / 2;
    
    // Search for empty space near center
    for (int r = 0; r < 20; r++) {
        for (int attempts = 0; attempts < 100; attempts++) {
            int tx = center_x + (rand() % (r * 2 + 1)) - r;
            int ty = center_y + (rand() % (r * 2 + 1)) - r;
            int tz = center_z + (rand() % (r * 2 + 1)) - r;
            
            if (tx > 0 && tx < cave->width - 1 &&
                ty > 0 && ty < cave->height - 1 &&
                tz > 0 && tz < cave->depth - 1) {
                if (cave->map[tz][ty][tx] == 0) {
                    *x = (float)tx / cave->width * 10.0f - 5.0f;
                    *y = (float)ty / cave->height * 10.0f - 5.0f;
                    *z = (float)tz / cave->depth * 10.0f - 5.0f;
                    return;
                }
            }
        }
    }
    
    // Fallback to center
    *x = 0.0f;
    *y = 0.0f;
    *z = 0.0f;
}

// Crystal generation
Crystal* generate_crystals(Cave* cave, int count) {
    Crystal* crystals = (Crystal*)malloc(count * sizeof(Crystal));
    
    for (int i = 0; i < count; i++) {
        // Find a suitable location
        int attempts = 0;
        while (attempts < 100) {
            int x = rand() % cave->width;
            int y = rand() % cave->height;
            int z = cave->depth / 2 + (rand() % 10 - 5);
            
            // Check if location is near a wall
            if (x > 0 && x < cave->width - 1 && y > 0 && y < cave->height - 1 && z > 0 && z < cave->depth - 1) {
                int wall_nearby = 0;
                for (int dz = -1; dz <= 1; dz++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            if (cave->map[z + dz][y + dy][x + dx] == 1) {
                                wall_nearby = 1;
                                break;
                            }
                        }
                    }
                }
                
                if (wall_nearby) {
                    crystals[i].x = (float)x / cave->width * 10.0f - 5.0f;
                    crystals[i].y = cave->height_map[y * cave->width + x] + 0.2f;
                    crystals[i].z = (float)y / cave->height * 10.0f - 5.0f;
                    crystals[i].size = 0.1f + (rand() % 100) / 200.0f;
                    crystals[i].rotation = (rand() % 360) * M_PI / 180.0f;
                    
                    // Random crystal colors
                    int color_type = rand() % 4;
                    switch (color_type) {
                        case 0:  // Blue
                            crystals[i].color[0] = 0.2f;
                            crystals[i].color[1] = 0.4f;
                            crystals[i].color[2] = 1.0f;
                            break;
                        case 1:  // Green
                            crystals[i].color[0] = 0.2f;
                            crystals[i].color[1] = 1.0f;
                            crystals[i].color[2] = 0.4f;
                            break;
                        case 2:  // Purple
                            crystals[i].color[0] = 0.8f;
                            crystals[i].color[1] = 0.2f;
                            crystals[i].color[2] = 1.0f;
                            break;
                        case 3:  // Orange
                            crystals[i].color[0] = 1.0f;
                            crystals[i].color[1] = 0.6f;
                            crystals[i].color[2] = 0.2f;
                            break;
                    }
                    
                    crystals[i].glow_intensity = 0.5f + (rand() % 50) / 100.0f;
                    break;
                }
            }
            attempts++;
        }
    }
    
    return crystals;
}

// Render crystals with basic shapes
void render_crystals(Crystal* crystals, int count) {
    for (int i = 0; i < count; i++) {
        glPushMatrix();
        
        glTranslatef(crystals[i].x, crystals[i].y, crystals[i].z);
        glRotatef(crystals[i].rotation * 180.0f / M_PI, 0, 1, 0);
        glScalef(crystals[i].size, crystals[i].size, crystals[i].size);
        
        // Set crystal color
        glColor3f(crystals[i].color[0], crystals[i].color[1], crystals[i].color[2]);
        
        // Draw simple pyramid crystal
        glBegin(GL_TRIANGLES);
        // Top pyramid
        glVertex3f( 0.0f,  0.8f,  0.0f);
        glVertex3f( 0.5f,  0.0f,  0.5f);
        glVertex3f(-0.5f,  0.0f,  0.5f);
        
        glVertex3f( 0.0f,  0.8f,  0.0f);
        glVertex3f(-0.5f,  0.0f,  0.5f);
        glVertex3f(-0.5f,  0.0f, -0.5f);
        
        glVertex3f( 0.0f,  0.8f,  0.0f);
        glVertex3f(-0.5f,  0.0f, -0.5f);
        glVertex3f( 0.5f,  0.0f, -0.5f);
        
        glVertex3f( 0.0f,  0.8f,  0.0f);
        glVertex3f( 0.5f,  0.0f, -0.5f);
        glVertex3f( 0.5f,  0.0f,  0.5f);
        glEnd();
        
        glPopMatrix();
    }
}

// Generate collectible gems
Gem* generate_gems(Cave* cave, int count) {
    Gem* gems = (Gem*)malloc(count * sizeof(Gem));
    
    for (int i = 0; i < count; i++) {
        // Find a spot near walls in empty space
        int attempts = 0;
        while (attempts < 100) {
            int x = rand() % cave->width;
            int y = rand() % cave->height;
            int z = rand() % cave->depth;
            
            // Check if this is empty space near a wall
            if (x > 1 && x < cave->width - 2 &&
                y > 1 && y < cave->height - 2 &&
                z > 1 && z < cave->depth - 2 &&
                cave->map[z][y][x] == 0) {
                
                // Check if there's a wall nearby
                int wall_nearby = 0;
                for (int dz = -1; dz <= 1 && !wall_nearby; dz++) {
                    for (int dy = -1; dy <= 1 && !wall_nearby; dy++) {
                        for (int dx = -1; dx <= 1 && !wall_nearby; dx++) {
                            if (cave->map[z + dz][y + dy][x + dx] == 1) {
                                wall_nearby = 1;
                            }
                        }
                    }
                }
                
                if (wall_nearby) {
                    gems[i].x = (float)x / cave->width * 10.0f - 5.0f;
                    gems[i].y = (float)y / cave->height * 10.0f - 5.0f;
                    gems[i].z = (float)z / cave->depth * 10.0f - 5.0f;
                    gems[i].rotation = (rand() % 360) * M_PI / 180.0f;
                    gems[i].bob_offset = (rand() % 100) / 100.0f * 2.0f * M_PI;
                    gems[i].type = rand() % 10;
                    gems[i].collected = 0;
                    gems[i].size = 0.1f + (rand() % 50) / 500.0f;
                    
                    // Set color based on type
                    switch (gems[i].type) {
                        case 0: // Ruby
                            gems[i].color[0] = 1.0f; gems[i].color[1] = 0.2f; gems[i].color[2] = 0.2f;
                            break;
                        case 1: // Emerald
                            gems[i].color[0] = 0.2f; gems[i].color[1] = 1.0f; gems[i].color[2] = 0.2f;
                            break;
                        case 2: // Sapphire
                            gems[i].color[0] = 0.2f; gems[i].color[1] = 0.2f; gems[i].color[2] = 1.0f;
                            break;
                        case 3: // Amethyst
                            gems[i].color[0] = 0.8f; gems[i].color[1] = 0.2f; gems[i].color[2] = 1.0f;
                            break;
                        case 4: // Topaz
                            gems[i].color[0] = 1.0f; gems[i].color[1] = 0.8f; gems[i].color[2] = 0.2f;
                            break;
                        case 5: // Diamond
                            gems[i].color[0] = 0.9f; gems[i].color[1] = 0.9f; gems[i].color[2] = 1.0f;
                            break;
                        case 6: // Onyx
                            gems[i].color[0] = 0.1f; gems[i].color[1] = 0.1f; gems[i].color[2] = 0.1f;
                            break;
                        case 7: // Aquamarine
                            gems[i].color[0] = 0.2f; gems[i].color[1] = 0.8f; gems[i].color[2] = 1.0f;
                            break;
                        case 8: // Citrine
                            gems[i].color[0] = 1.0f; gems[i].color[1] = 0.6f; gems[i].color[2] = 0.0f;
                            break;
                        case 9: // Rose Quartz
                            gems[i].color[0] = 1.0f; gems[i].color[1] = 0.6f; gems[i].color[2] = 0.8f;
                            break;
                    }
                    break;
                }
            }
            attempts++;
        }
    }
    
    return gems;
}

// Render gems with rotation and bobbing animation
void render_gems(Gem* gems, int count, float time) {
    for (int i = 0; i < count; i++) {
        if (gems[i].collected) continue;
        
        float bob = sin(time * 2.0f + gems[i].bob_offset) * 0.05f;
        float rotation = time + gems[i].rotation;
        
        glPushMatrix();
        
        glTranslatef(gems[i].x, gems[i].y + bob, gems[i].z);
        glRotatef(rotation * 180.0f / M_PI, 0, 1, 0);
        glScalef(gems[i].size, gems[i].size, gems[i].size);
        
        // Set gem color
        glColor3f(gems[i].color[0], gems[i].color[1], gems[i].color[2]);
        
        // Draw octahedron gem
        glBegin(GL_TRIANGLES);
        // Top pyramid
        glVertex3f( 0.0f,  0.5f,  0.0f);
        glVertex3f( 0.5f,  0.0f,  0.0f);
        glVertex3f( 0.0f,  0.0f,  0.5f);
        
        glVertex3f( 0.0f,  0.5f,  0.0f);
        glVertex3f( 0.0f,  0.0f,  0.5f);
        glVertex3f(-0.5f,  0.0f,  0.0f);
        
        glVertex3f( 0.0f,  0.5f,  0.0f);
        glVertex3f(-0.5f,  0.0f,  0.0f);
        glVertex3f( 0.0f,  0.0f, -0.5f);
        
        glVertex3f( 0.0f,  0.5f,  0.0f);
        glVertex3f( 0.0f,  0.0f, -0.5f);
        glVertex3f( 0.5f,  0.0f,  0.0f);
        
        // Bottom pyramid
        glVertex3f( 0.0f, -0.5f,  0.0f);
        glVertex3f( 0.0f,  0.0f,  0.5f);
        glVertex3f( 0.5f,  0.0f,  0.0f);
        
        glVertex3f( 0.0f, -0.5f,  0.0f);
        glVertex3f(-0.5f,  0.0f,  0.0f);
        glVertex3f( 0.0f,  0.0f,  0.5f);
        
        glVertex3f( 0.0f, -0.5f,  0.0f);
        glVertex3f( 0.0f,  0.0f, -0.5f);
        glVertex3f(-0.5f,  0.0f,  0.0f);
        
        glVertex3f( 0.0f, -0.5f,  0.0f);
        glVertex3f( 0.5f,  0.0f,  0.0f);
        glVertex3f( 0.0f,  0.0f, -0.5f);
        glEnd();
        
        glPopMatrix();
    }
}

// Check for gem collection
int collect_gem(Gem* gems, int count, float player_x, float player_y, float player_z, float collect_radius) {
    for (int i = 0; i < count; i++) {
        if (!gems[i].collected) {
            float dx = gems[i].x - player_x;
            float dy = gems[i].y - player_y;
            float dz = gems[i].z - player_z;
            float dist = sqrt(dx*dx + dy*dy + dz*dz);
            
            if (dist < collect_radius) {
                gems[i].collected = 1;
                return gems[i].type;
            }
        }
    }
    return -1;
}

// Respawn a collected gem at a new location
void respawn_gem(Gem* gem, Cave* cave) {
    // Find new location
    int attempts = 0;
    while (attempts < 100) {
        int x = rand() % cave->width;
        int y = rand() % cave->height;
        int z = rand() % cave->depth;
        
        if (x > 1 && x < cave->width - 2 &&
            y > 1 && y < cave->height - 2 &&
            z > 1 && z < cave->depth - 2 &&
            cave->map[z][y][x] == 0) {
            
            gem->x = (float)x / cave->width * 10.0f - 5.0f;
            gem->y = (float)y / cave->height * 10.0f - 5.0f;
            gem->z = (float)z / cave->depth * 10.0f - 5.0f;
            gem->collected = 0;
            gem->bob_offset = (rand() % 100) / 100.0f * 2.0f * M_PI;
            break;
        }
        attempts++;
    }
}

// Cave mesh creation and rendering
CaveMesh* create_cave_mesh(Cave* cave) {
    CaveMesh* mesh = (CaveMesh*)calloc(1, sizeof(CaveMesh));
    
    // Generate VAO and VBOs
    glGenVertexArrays(1, &mesh->vao);
    glGenBuffers(1, &mesh->vbo_vertices);
    glGenBuffers(1, &mesh->vbo_normals);
    glGenBuffers(1, &mesh->vbo_texcoords);
    glGenBuffers(1, &mesh->ebo);
    
    // Create simple mesh based on cave dimensions
    int vertex_count = cave->width * cave->height;
    float* vertices = (float*)malloc(vertex_count * 3 * sizeof(float));
    float* normals = (float*)malloc(vertex_count * 3 * sizeof(float));
    float* texcoords = (float*)malloc(vertex_count * 2 * sizeof(float));
    
    // Generate vertices
    int v_idx = 0;
    for (int z = 0; z < cave->height; z++) {
        for (int x = 0; x < cave->width; x++) {
            // Position
            vertices[v_idx * 3 + 0] = (float)x / cave->width * 10.0f - 5.0f;
            vertices[v_idx * 3 + 1] = cave->height_map[z * cave->width + x];
            vertices[v_idx * 3 + 2] = (float)z / cave->height * 10.0f - 5.0f;
            
            // Normal
            normals[v_idx * 3 + 0] = 0.0f;
            normals[v_idx * 3 + 1] = 1.0f;
            normals[v_idx * 3 + 2] = 0.0f;
            
            // Texture coordinates
            texcoords[v_idx * 2 + 0] = (float)x / cave->width;
            texcoords[v_idx * 2 + 1] = (float)z / cave->height;
            
            v_idx++;
        }
    }
    
    // Generate indices
    mesh->index_count = (cave->width - 1) * (cave->height - 1) * 6;
    unsigned int* indices = (unsigned int*)malloc(mesh->index_count * sizeof(unsigned int));
    
    int idx = 0;
    for (int z = 0; z < cave->height - 1; z++) {
        for (int x = 0; x < cave->width - 1; x++) {
            int base = z * cave->width + x;
            
            // First triangle
            indices[idx++] = base;
            indices[idx++] = base + 1;
            indices[idx++] = base + cave->width;
            
            // Second triangle
            indices[idx++] = base + 1;
            indices[idx++] = base + cave->width + 1;
            indices[idx++] = base + cave->width;
        }
    }
    
    // Upload to GPU
    glBindVertexArray(mesh->vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_vertices);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_normals);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), normals, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_texcoords);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * 2 * sizeof(float), texcoords, GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(2);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->index_count * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    
    glBindVertexArray(0);
    
    // Create textures from cave data
    mesh->height_texture = create_texture_from_data(cave->height_map, cave->width, cave->height, 1);
    mesh->normal_texture = create_texture_from_data(cave->normal_map, cave->width, cave->height, 3);
    
    // Generate procedural textures
    mesh->diffuse_texture = generate_rock_texture(512, 512);
    mesh->roughness_texture = generate_roughness_texture(512, 512);
    mesh->ao_texture = generate_ao_texture(512, 512);
    mesh->emissive_texture = generate_crystal_emissive_texture(512, 512);
    
    // Cleanup
    free(vertices);
    free(normals);
    free(texcoords);
    free(indices);
    
    return mesh;
}

void free_cave_mesh(CaveMesh* mesh) {
    if (mesh) {
        glDeleteVertexArrays(1, &mesh->vao);
        glDeleteBuffers(1, &mesh->vbo_vertices);
        glDeleteBuffers(1, &mesh->vbo_normals);
        glDeleteBuffers(1, &mesh->vbo_texcoords);
        glDeleteBuffers(1, &mesh->ebo);
        glDeleteTextures(1, &mesh->height_texture);
        glDeleteTextures(1, &mesh->normal_texture);
        glDeleteTextures(1, &mesh->diffuse_texture);
        glDeleteTextures(1, &mesh->roughness_texture);
        glDeleteTextures(1, &mesh->ao_texture);
        glDeleteTextures(1, &mesh->emissive_texture);
        free(mesh);
    }
}

void render_cave_with_tessellation(CaveMesh* mesh) {
    // Simple fallback rendering without tessellation
    glBindVertexArray(mesh->vao);
    
    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mesh->height_texture);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mesh->normal_texture);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mesh->diffuse_texture);
    
    // Render as triangles for compatibility
    glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
    
    glBindVertexArray(0);
}

// Render cave interior walls
void render_cave_interior(Cave* cave, float cam_x, float cam_y, float cam_z) {
    // Convert camera position to cave coordinates
    int cx = (int)((cam_x + 5.0f) / 10.0f * cave->width);
    int cy = (int)((cam_y + 5.0f) / 10.0f * cave->height);
    int cz = (int)((cam_z + 5.0f) / 10.0f * cave->depth);
    
    // Render nearby cave blocks
    int render_dist = 25;  // Increased render distance
    
    glUseProgram(0);  // Use fixed function for now
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    // Set up better lighting for cave interior
    float light_pos[] = {cam_x, cam_y + 1.0f, cam_z, 1.0f};
    float light_ambient[] = {0.4f, 0.4f, 0.5f, 1.0f};   // Increased ambient
    float light_diffuse[] = {0.8f, 0.8f, 0.9f, 1.0f};   // Brighter diffuse
    float light_specular[] = {0.3f, 0.3f, 0.4f, 1.0f};  // Some specular
    
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    
    glBegin(GL_QUADS);
    
    for (int z = cz - render_dist; z <= cz + render_dist; z++) {
        for (int y = cy - render_dist; y <= cy + render_dist; y++) {
            for (int x = cx - render_dist; x <= cx + render_dist; x++) {
                if (x >= 0 && x < cave->width &&
                    y >= 0 && y < cave->height &&
                    z >= 0 && z < cave->depth) {
                    
                    if (cave->map[z][y][x] == 1) {
                        // This is a wall block - check if we need to render any faces
                        float bx = (float)x / cave->width * 10.0f - 5.0f;
                        float by = (float)y / cave->height * 10.0f - 5.0f;
                        float bz = (float)z / cave->depth * 10.0f - 5.0f;
                        float s = 0.05f;  // Block size
                        
                        // Only render faces adjacent to empty space
                        if (x > 0 && cave->map[z][y][x-1] == 0) {
                            // Left face - brighter colors
                            glNormal3f(-1, 0, 0);
                            glColor3f(0.6f, 0.5f, 0.4f);  // Increased brightness
                            glVertex3f(bx-s, by-s, bz-s);
                            glVertex3f(bx-s, by-s, bz+s);
                            glVertex3f(bx-s, by+s, bz+s);
                            glVertex3f(bx-s, by+s, bz-s);
                        }
                        
                        if (x < cave->width-1 && cave->map[z][y][x+1] == 0) {
                            // Right face
                            glNormal3f(1, 0, 0);
                            glColor3f(0.6f, 0.5f, 0.4f);
                            glVertex3f(bx+s, by-s, bz+s);
                            glVertex3f(bx+s, by-s, bz-s);
                            glVertex3f(bx+s, by+s, bz-s);
                            glVertex3f(bx+s, by+s, bz+s);
                        }
                        
                        if (y > 0 && cave->map[z][y-1][x] == 0) {
                            // Bottom face - darker for ground
                            glNormal3f(0, -1, 0);
                            glColor3f(0.5f, 0.4f, 0.3f);
                            glVertex3f(bx-s, by-s, bz+s);
                            glVertex3f(bx-s, by-s, bz-s);
                            glVertex3f(bx+s, by-s, bz-s);
                            glVertex3f(bx+s, by-s, bz+s);
                        }
                        
                        if (y < cave->height-1 && cave->map[z][y+1][x] == 0) {
                            // Top face - brightest for ceiling
                            glNormal3f(0, 1, 0);
                            glColor3f(0.7f, 0.6f, 0.5f);
                            glVertex3f(bx-s, by+s, bz-s);
                            glVertex3f(bx-s, by+s, bz+s);
                            glVertex3f(bx+s, by+s, bz+s);
                            glVertex3f(bx+s, by+s, bz-s);
                        }
                        
                        if (z > 0 && cave->map[z-1][y][x] == 0) {
                            // Front face
                            glNormal3f(0, 0, -1);
                            glColor3f(0.55f, 0.45f, 0.35f);
                            glVertex3f(bx-s, by-s, bz-s);
                            glVertex3f(bx+s, by-s, bz-s);
                            glVertex3f(bx+s, by+s, bz-s);
                            glVertex3f(bx-s, by+s, bz-s);
                        }
                        
                        if (z < cave->depth-1 && cave->map[z+1][y][x] == 0) {
                            // Back face
                            glNormal3f(0, 0, 1);
                            glColor3f(0.55f, 0.45f, 0.35f);
                            glVertex3f(bx+s, by-s, bz+s);
                            glVertex3f(bx-s, by-s, bz+s);
                            glVertex3f(bx-s, by+s, bz+s);
                            glVertex3f(bx+s, by+s, bz+s);
                        }
                    }
                }
            }
        }
    }
    
    glEnd();
    glDisable(GL_LIGHTING);
}

// Texture generation functions
GLuint generate_rock_texture(int width, int height) {
    float* data = (float*)malloc(width * height * 3 * sizeof(float));
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float noise1 = fractal_noise_3d(x * 0.01f, y * 0.01f, 0.0f, 5, 0.5f);
            float noise2 = fractal_noise_3d(x * 0.05f, y * 0.05f, 10.0f, 3, 0.3f);
            
            float value = 0.3f + noise1 * 0.2f + noise2 * 0.1f;
            value = fmax(0.0f, fmin(1.0f, value));
            
            int idx = (y * width + x) * 3;
            data[idx + 0] = value * 0.4f;
            data[idx + 1] = value * 0.35f;
            data[idx + 2] = value * 0.3f;
        }
    }
    
    GLuint texture = create_texture_from_data(data, width, height, 3);
    free(data);
    return texture;
}

GLuint generate_roughness_texture(int width, int height) {
    float* data = (float*)malloc(width * height * sizeof(float));
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float noise = fractal_noise_3d(x * 0.02f, y * 0.02f, 0.0f, 4, 0.6f);
            data[y * width + x] = 0.7f + noise * 0.3f;
        }
    }
    
    GLuint texture = create_texture_from_data(data, width, height, 1);
    free(data);
    return texture;
}

GLuint generate_ao_texture(int width, int height) {
    float* data = (float*)malloc(width * height * sizeof(float));
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float noise = fractal_noise_3d(x * 0.01f, y * 0.01f, 0.0f, 3, 0.5f);
            data[y * width + x] = 0.8f + noise * 0.2f;
        }
    }
    
    GLuint texture = create_texture_from_data(data, width, height, 1);
    free(data);
    return texture;
}

GLuint generate_crystal_emissive_texture(int width, int height) {
    float* data = (float*)calloc(width * height * 3, sizeof(float));
    
    // Add some scattered emissive spots
    for (int i = 0; i < 50; i++) {
        int cx = rand() % width;
        int cy = rand() % height;
        float intensity = (rand() % 100) / 100.0f;
        
        for (int y = cy - 10; y < cy + 10; y++) {
            for (int x = cx - 10; x < cx + 10; x++) {
                if (x >= 0 && x < width && y >= 0 && y < height) {
                    float dist = sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
                    if (dist < 10) {
                        float value = intensity * (1.0f - dist / 10.0f);
                        int idx = (y * width + x) * 3;
                        data[idx + 0] += value * 0.2f;
                        data[idx + 1] += value * 0.5f;
                        data[idx + 2] += value * 1.0f;
                    }
                }
            }
        }
    }
    
    GLuint texture = create_texture_from_data(data, width, height, 3);
    free(data);
    return texture;
}

GLuint create_texture_from_data(const float* data, int width, int height, int channels) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    GLenum format = GL_RED;
    if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;
    
    glTexImage2D(GL_TEXTURE_2D, 0, format == GL_RED ? GL_R32F : GL_RGB32F,
                 width, height, 0, format, GL_FLOAT, data);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glGenerateMipmap(GL_TEXTURE_2D);
    
    return texture;
}
