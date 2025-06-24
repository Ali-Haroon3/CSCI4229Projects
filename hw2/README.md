# HW2 - 3D Scene Viewer
**Ali Haroon**  
**CSCI 4229 Computer Graphics**

## Program Description
This program displays a 3D town scene consisting of houses, trees, and windmills on a grass plane. The scene can be viewed from multiple perspectives and projection modes.

## Building the Program
To compile the program, simply run:
```
make
```


## Running the Program
Execute the program with:
```
./hw2
```

## Key Bindings

### View Mode Selection
- **1** - Switch to Oblique Orthogonal projection
- **2** - Switch to Oblique Perspective projection  
- **3** - Switch to First Person perspective

### Oblique View Navigation (modes 1 and 2)
- **Arrow Keys** - Move the camera position around the scene
  - UP/DOWN arrows move camera vertically
  - LEFT/RIGHT arrows move camera horizontally

### First Person Navigation (mode 3)
- **WASD** - Move through the scene
  - W - Move forward
  - S - Move backward
  - A - Strafe left
  - D - Strafe right
- **Arrow Keys** - Look around
  - LEFT/RIGHT arrows - Turn left/right
  - UP/DOWN arrows - Look up/down

### General Controls
- **ESC** or **Q** - Quit the program

## Scene Objects

The scene contains multiple instances of three generic objects:

1. **Houses** - Rectangular buildings with triangular roofs

2. **Trees** - Cylinder tree trunks and sphere leaves/top

3. **Windmills** - Towers with blades

Green groudn to represent grass

## Implementation Notes

- Objects are created using generic functions and transformed with glTranslatef, glRotatef, and glScalef
- Scene uses proper depth testing to handle object occlusion
- All three projection modes show the same scene from appropriate viewpoints
- First person mode includes collision-free navigation
- Window can be resized and maintains proper aspect ratio

## Time Spent
Approximately 15 hours total:
Most of my time was looking back at class notes and seeing how to make objects. I was also confused if we were supposed to use lighting or not, it's still very confusing to me and I don't know how to use it. I then saw tutorials on how to create houses and trees online and they are in the references below. I also found different point of view references for OpenGL

## Code References
https://www.youtube.com/watch?v=q5jOLztcvsM 
Spheres: https://www.reddit.com/r/opengl/comments/jondor/how_would_you_render_a_sphere_in_modern_opengl_45/ 
Windmill (Much more complicated than mine) https://github.com/ping543f/Computer-Graphics--OpenGL-GLUT/blob/master/windmillProject.cpp
Makefile issues fix from last homework I did wrong: https://www.youtube.com/watch?v=H6EsqLUXwv4 
Orthographic projections: https://www.youtube.com/watch?v=j4sloLs-9sE 
Camera: https://learnopengl.com/Getting-started/Camera 
