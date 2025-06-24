/*
 * Ali Haroon
 * CSCI 4229 HW2
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

int window_width = 800;
int window_height = 600;
int view_mode = 0;
double view_angle = 55.0;
double aspect_ratio = 1.0;
double near_plane = 0.1;
double far_plane = 100.0;

double eye_x = 5.0;
double eye_y = 5.0;
double eye_z = 15.0;
double center_x = 0.0;
double center_y = 0.0;
double center_z = 0.0;
double up_x = 0.0;
double up_y = 1.0;
double up_z = 0.0;

double fp_x = 0.0;
double fp_y = 2.0;
double fp_z = 8.0;
double fp_angle = 0.0;
double fp_elevation = 0.0;

double ortho_dim = 20.0;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void fatal_error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(1);
}

void print_text(const char* format, ...)
{
    char buffer[1024];
    char* position;
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    for (position = buffer; *position; position = position + 1)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *position);
    }
}

void check_errors(const char* where)
{
    int error_code = glGetError();
    if (error_code)
    {
        fprintf(stderr, "ERROR: %s [%s]\n", gluErrorString(error_code), where);
    }
}

//house
void draw_house_shape(double width, double height, double depth)
{
    glBegin(GL_QUADS);
    
    // Fr
    glColor3f(0.8, 0.6, 0.4);
    glVertex3f(-width/2, 0, depth/2);
    glVertex3f(width/2, 0, depth/2);
    glVertex3f(width/2, height, depth/2);
    glVertex3f(-width/2, height, depth/2);
    
    // Back
    glVertex3f(width/2, 0, -depth/2);
    glVertex3f(-width/2, 0, -depth/2);
    glVertex3f(-width/2, height, -depth/2);
    glVertex3f(width/2, height, -depth/2);
    
    // L
    glVertex3f(-width/2, 0, -depth/2);
    glVertex3f(-width/2, 0, depth/2);
    glVertex3f(-width/2, height, depth/2);
    glVertex3f(-width/2, height, -depth/2);
    
    // R
    glVertex3f(width/2, 0, depth/2);
    glVertex3f(width/2, 0, -depth/2);
    glVertex3f(width/2, height, -depth/2);
    glVertex3f(width/2, height, depth/2);
    
    // Bottom
    glColor3f(0.5, 0.3, 0.2);
    glVertex3f(-width/2, 0, -depth/2);
    glVertex3f(width/2, 0, -depth/2);
    glVertex3f(width/2, 0, depth/2);
    glVertex3f(-width/2, 0, depth/2);
    
    glEnd();
    
    // Roof
    glBegin(GL_TRIANGLES);
    glColor3f(0.6, 0.2, 0.2);
    
    // FR
    glVertex3f(-width/2, height, depth/2);
    glVertex3f(width/2, height, depth/2);
    glVertex3f(0, height + 1.0, depth/2);
    
    // Back
    glVertex3f(width/2, height, -depth/2);
    glVertex3f(-width/2, height, -depth/2);
    glVertex3f(0, height + 1.0, -depth/2);
    
    glEnd();
    
    // Side
    glBegin(GL_QUADS);

    
    glVertex3f(-width/2, height, depth/2);
    glVertex3f(0, height + 1.0, depth/2);
    glVertex3f(0, height + 1.0, -depth/2);
    glVertex3f(-width/2, height, -depth/2);
    
    glVertex3f(0, height + 1.0, depth/2);
    glVertex3f(width/2, height, depth/2);
    glVertex3f(width/2, height, -depth/2);
    glVertex3f(0, height + 1.0, -depth/2);
    
    glEnd();
}


void draw_tree_shape(double trunk_radius, double trunk_height, double crown_radius)
{
    int slices = 12;
    int i;
    double angle_step = 2.0 * M_PI / slices;
    
    glColor3f(0.4, 0.2, 0.1);
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= slices; i = i + 1)
    {
        double angle = i * angle_step;
        double x = trunk_radius * cos(angle);
        double z = trunk_radius * sin(angle);
        glVertex3f(x, 0, z);
        glVertex3f(x, trunk_height, z);
    }
    glEnd();
    
    glColor3f(0.2, 0.6, 0.2);
    glPushMatrix();
    glTranslatef(0, trunk_height + crown_radius * 0.7, 0);
    
    int latitude_segments = 8;
    int longitude_segments = 12;
    int lat, lon;
    
    glBegin(GL_TRIANGLES);
    for (lat = 0; lat < latitude_segments; lat = lat + 1)
    {
        double lat1 = M_PI * (-0.5 + (double)(lat) / latitude_segments);
        double lat2 = M_PI * (-0.5 + (double)(lat + 1) / latitude_segments);
        
        for (lon = 0; lon < longitude_segments; lon = lon + 1)
        {
            double lon1 = 2.0 * M_PI * (double)(lon) / longitude_segments;
            double lon2 = 2.0 * M_PI * (double)(lon + 1) / longitude_segments;
            
            double x1 = crown_radius * cos(lat1) * cos(lon1);
            double y1 = crown_radius * sin(lat1);
            double z1 = crown_radius * cos(lat1) * sin(lon1);
            
            double x2 = crown_radius * cos(lat2) * cos(lon1);
            double y2 = crown_radius * sin(lat2);
            double z2 = crown_radius * cos(lat2) * sin(lon1);
            
            double x3 = crown_radius * cos(lat1) * cos(lon2);
            double y3 = crown_radius * sin(lat1);
            double z3 = crown_radius * cos(lat1) * sin(lon2);
            
            double x4 = crown_radius * cos(lat2) * cos(lon2);
            double y4 = crown_radius * sin(lat2);
            double z4 = crown_radius * cos(lat2) * sin(lon2);
            
            glVertex3f(x1, y1, z1);
            glVertex3f(x2, y2, z2);
            glVertex3f(x3, y3, z3);
            
            glVertex3f(x2, y2, z2);
            glVertex3f(x4, y4, z4);
            glVertex3f(x3, y3, z3);
        }
    }
    glEnd();
    
    glPopMatrix();
}

void draw_windmill_shape(double tower_radius, double tower_height, double blade_length)
{
    int slices = 8;
    int i;
    double angle_step = 2.0 * M_PI / slices;
    
    glColor3f(0.7, 0.7, 0.7);
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= slices; i = i + 1)
    {
        double angle = i * angle_step;
        double x_bottom = tower_radius * cos(angle);
        double z_bottom = tower_radius * sin(angle);
        double x_top = (tower_radius * 0.6) * cos(angle);
        double z_top = (tower_radius * 0.6) * sin(angle);
        glVertex3f(x_bottom, 0, z_bottom);
        glVertex3f(x_top, tower_height, z_top);
    }
    glEnd();
    
    glPushMatrix();
    glTranslatef(0, tower_height - 1.0, tower_radius);
    
    glColor3f(0.9, 0.9, 0.9);
    
    for (i = 0; i < 4; i = i + 1)
    {
        glPushMatrix();
        glRotatef(i * 90.0, 0, 0, 1);
        
        glBegin(GL_TRIANGLES);
        glVertex3f(0, 0, 0);
        glVertex3f(blade_length, 0.2, 0);
        glVertex3f(blade_length, -0.2, 0);
        glEnd();
        
        glPopMatrix();
    }
    
    glPopMatrix();
}

void draw_ground_plane()
{
    glColor3f(0.3, 0.6, 0.3);
    glBegin(GL_QUADS);
    glVertex3f(-50, 0, -50);
    glVertex3f(50, 0, -50);
    glVertex3f(50, 0, 50);
    glVertex3f(-50, 0, 50);
    glEnd();
}

void setup_projection()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    if (view_mode == 0)
    {
        glOrtho(-ortho_dim * aspect_ratio, ortho_dim * aspect_ratio,
                -ortho_dim, ortho_dim, near_plane, far_plane);
    }
    else if (view_mode == 1)
    {
        gluPerspective(view_angle, aspect_ratio, near_plane, far_plane);
    }
    else if (view_mode == 2)
    {
        gluPerspective(view_angle, aspect_ratio, near_plane, far_plane);
    }
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void setup_view()
{
    if (view_mode == 0 || view_mode == 1)
    {
        gluLookAt(eye_x, eye_y, eye_z,
                  center_x, center_y, center_z,
                  up_x, up_y, up_z);
    }
    else if (view_mode == 2)
    {
        double look_x = fp_x + cos(fp_angle * M_PI / 180.0) * cos(fp_elevation * M_PI / 180.0);
        double look_y = fp_y + sin(fp_elevation * M_PI / 180.0);
        double look_z = fp_z + sin(fp_angle * M_PI / 180.0) * cos(fp_elevation * M_PI / 180.0);
        
        gluLookAt(fp_x, fp_y, fp_z,
                  look_x, look_y, look_z,
                  0, 1, 0);
    }
}

void display_scene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    
    setup_projection();
    setup_view();
    
    draw_ground_plane();
    
    glPushMatrix();
    glTranslatef(-5, 0, -3);
    glScalef(1.2, 1.0, 0.8);
    draw_house_shape(2.0, 3.0, 2.5);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(3, 0, -6);
    glScalef(0.8, 1.5, 1.2);
    glRotatef(45, 0, 1, 0);
    draw_house_shape(2.0, 3.0, 2.5);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-8, 0, 4);
    glScalef(1.0, 0.8, 1.0);
    glRotatef(-30, 0, 1, 0);
    draw_house_shape(2.0, 3.0, 2.5);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(7, 0, 2);
    glScalef(1.4, 1.2, 1.0);
    glRotatef(120, 0, 1, 0);
    draw_house_shape(2.0, 3.0, 2.5);
    glPopMatrix();
    
    // Draw multiple trees at different positions and scales
    glPushMatrix();
    glTranslatef(-2, 0, 8);
    glScalef(1.0, 1.2, 1.0);
    draw_tree_shape(0.3, 2.5, 1.5);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(5, 0, 8);
    glScalef(0.8, 1.0, 0.8);
    draw_tree_shape(0.3, 2.5, 1.5);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-10, 0, -8);
    glScalef(1.3, 1.4, 1.3);
    draw_tree_shape(0.3, 2.5, 1.5);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(10, 0, -2);
    glScalef(0.9, 0.8, 0.9);
    draw_tree_shape(0.3, 2.5, 1.5);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-12, 0, 0);
    glScalef(1.0, 1.0, 1.0);
    glRotatef(15, 0, 1, 0);
    draw_windmill_shape(0.5, 8.0, 3.0);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(12, 0, -8);
    glScalef(1.2, 0.9, 1.2);
    glRotatef(-45, 0, 1, 0);
    draw_windmill_shape(0.5, 8.0, 3.0);
    glPopMatrix();
    
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, window_width, 0, window_height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glColor3f(1.0, 1.0, 1.0);
    glRasterPos2f(10, window_height - 30);
    
    if (view_mode == 0)
    {
        print_text("View Mode: Oblique Orthogonal (Press 1/2/3 to change)");
    }
    else if (view_mode == 1)
    {
        print_text("View Mode: Oblique Perspective (Press 1/2/3 to change)");
    }
    else if (view_mode == 2)
    {
        print_text("View Mode: First Person (WASD to move, arrows to look)");
    }
    
    glRasterPos2f(10, window_height - 55);
    print_text("ESC or Q to quit");
    
    check_errors("display_scene");
    glutSwapBuffers();
}

void handle_reshape(int width, int height)
{
    window_width = width;
    window_height = height;
    aspect_ratio = (double)width / (double)height;
    glViewport(0, 0, width, height);
    glutPostRedisplay();
}

void handle_keyboard(unsigned char key, int x, int y)
{
    if (key == 27 || key == 'q' || key == 'Q')
    {
        exit(0);
    }
    else if (key == '1')
    {
        view_mode = 0;
        glutPostRedisplay();
    }
    else if (key == '2')
    {
        view_mode = 1;
        glutPostRedisplay();
    }
    else if (key == '3')
    {
        view_mode = 2;
        glutPostRedisplay();
    }
    else if (view_mode == 2)
    {
        // First person movement
        if (key == 'w' || key == 'W')
        {
            fp_x = fp_x + cos(fp_angle * M_PI / 180.0) * 0.5;
            fp_z = fp_z + sin(fp_angle * M_PI / 180.0) * 0.5;
            glutPostRedisplay();
        }
        else if (key == 's' || key == 'S')
        {
            fp_x = fp_x - cos(fp_angle * M_PI / 180.0) * 0.5;
            fp_z = fp_z - sin(fp_angle * M_PI / 180.0) * 0.5;
            glutPostRedisplay();
        }
        else if (key == 'a' || key == 'A')
        {
            fp_x = fp_x - cos((fp_angle + 90) * M_PI / 180.0) * 0.5;
            fp_z = fp_z - sin((fp_angle + 90) * M_PI / 180.0) * 0.5;
            glutPostRedisplay();
        }
        else if (key == 'd' || key == 'D')
        {
            fp_x = fp_x + cos((fp_angle + 90) * M_PI / 180.0) * 0.5;
            fp_z = fp_z + sin((fp_angle + 90) * M_PI / 180.0) * 0.5;
            glutPostRedisplay();
        }
    }
}

void handle_special_keys(int key, int x, int y)
{
    if (view_mode == 0 || view_mode == 1)
    {
        // Oblique view controls
        if (key == GLUT_KEY_UP)
        {
            eye_y = eye_y + 0.5;
            glutPostRedisplay();
        }
        else if (key == GLUT_KEY_DOWN)
        {
            eye_y = eye_y - 0.5;
            glutPostRedisplay();
        }
        else if (key == GLUT_KEY_LEFT)
        {
            eye_x = eye_x - 0.5;
            glutPostRedisplay();
        }
        else if (key == GLUT_KEY_RIGHT)
        {
            eye_x = eye_x + 0.5;
            glutPostRedisplay();
        }
    }
    else if (view_mode == 2)
    {
        // First person POV VIew
        if (key == GLUT_KEY_LEFT)
        {
            fp_angle = fp_angle - 5.0;
            glutPostRedisplay();
        }
        else if (key == GLUT_KEY_RIGHT)
        {
            fp_angle = fp_angle + 5.0;
            glutPostRedisplay();
        }
        else if (key == GLUT_KEY_UP)
        {
            fp_elevation = fp_elevation + 5.0;
            if (fp_elevation > 90) fp_elevation = 90;
            glutPostRedisplay();
        }
        else if (key == GLUT_KEY_DOWN)
        {
            fp_elevation = fp_elevation - 5.0;
            if (fp_elevation < -90) fp_elevation = -90;
            glutPostRedisplay();
        }
    }
}

int main(int argc, char* argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("Ali Haroon");
    
    glutDisplayFunc(display_scene);
    glutReshapeFunc(handle_reshape);
    glutKeyboardFunc(handle_keyboard);
    glutSpecialFunc(handle_special_keys);
    
    glClearColor(0.5, 0.8, 1.0, 1.0);
    glEnable(GL_DEPTH_TEST);
    
    glutMainLoop();
    return 0;
}
