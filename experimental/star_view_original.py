import csv
import math
from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *

# ---- Camera and rotation state ----
camera_distance = 50.0
camera_angle_x = 0.0
camera_angle_y = 0.0
mouse_last_x = 0
mouse_last_y = 0
mouse_left_down = False

stars = []

# ---- Load stars from CSV ----
def load_stars(filename):
    global stars
    stars = []
    with open(filename, newline='') as f:
        reader = csv.reader(f)
        for row in reader:
            name = row[0]
            ra_h = int(row[1])
            ra_m = int(row[2])
            ra_s = float(row[3])
            dec_d = int(row[4])
            dec_m = int(row[5])
            dec_s = float(row[6])
            dist_ly = float(row[7])  # distance in light-years

            # Convert RA (hms) to degrees
            ra_deg = (ra_h + ra_m/60 + ra_s/3600) * 15.0
            # Convert Dec (dms) to degrees
            dec_deg = dec_d + (dec_m/60 if dec_d >= 0 else -dec_m/60) + (dec_s/3600 if dec_d >= 0 else -dec_s/3600)
            
            # Convert to radians
            ra_rad = math.radians(ra_deg)
            dec_rad = math.radians(dec_deg)
            
            # Convert spherical to Cartesian coordinates
            x = dist_ly * math.cos(dec_rad) * math.cos(ra_rad)
            y = dist_ly * math.cos(dec_rad) * math.sin(ra_rad)
            z = dist_ly * math.sin(dec_rad)

            stars.append((x, y, z, name))

# ---- OpenGL display ----
def display():
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    glLoadIdentity()
    
    # Camera transform
    glTranslatef(0.0, 0.0, -camera_distance)
    glRotatef(camera_angle_x, 1, 0, 0)
    glRotatef(camera_angle_y, 0, 1, 0)
    
    # Draw stars
    glPointSize(2)
    glBegin(GL_POINTS)
    glColor3f(1.0, 1.0, 1.0)
    for x, y, z, name in stars:
        glVertex3f(x, y, z)
    glEnd()
    
    glutSwapBuffers()

# ---- Window reshape ----
def reshape(width, height):
    glViewport(0, 0, width, height)
    glMatrixMode(GL_PROJECTION)
    glLoadIdentity()
    gluPerspective(45.0, float(width)/float(height), 0.1, 10000.0)
    glMatrixMode(GL_MODELVIEW)
    glLoadIdentity()

# ---- Mouse drag for rotation ----
def mouse(button, state, x, y):
    global mouse_left_down, mouse_last_x, mouse_last_y
    if button == GLUT_LEFT_BUTTON:
        if state == GLUT_DOWN:
            mouse_left_down = True
            mouse_last_x = x
            mouse_last_y = y
        elif state == GLUT_UP:
            mouse_left_down = False

def motion(x, y):
    global mouse_last_x, mouse_last_y, camera_angle_x, camera_angle_y
    if mouse_left_down:
        dx = x - mouse_last_x
        dy = y - mouse_last_y
        camera_angle_y += dx * 0.5
        camera_angle_x += dy * 0.5
        mouse_last_x = x
        mouse_last_y = y
        glutPostRedisplay()

# ---- Mouse wheel for zoom ----
def mouse_wheel(button, dir, x, y):
    global camera_distance
    if dir > 0:
        camera_distance *= 0.9  # zoom in
    else:
        camera_distance *= 1.1  # zoom out
    glutPostRedisplay()

# ---- Initialization ----
def init():
    glEnable(GL_DEPTH_TEST)
    glClearColor(0, 0, 0, 1)

# ---- Main ----
if __name__ == "__main__":
    load_stars("stars.csv")
    
    glutInit()
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH)
    glutInitWindowSize(800, 600)
    glutCreateWindow(b"3D Star Map")
    
    init()
    
    glutDisplayFunc(display)
    glutReshapeFunc(reshape)
    glutMouseFunc(mouse)
    glutMotionFunc(motion)
    
    # Some GLUT versions need this for the wheel
    try:
        glutMouseWheelFunc(mouse_wheel)
    except:
        print("Mouse wheel may not be supported; zoom manually by changing camera_distance")
    
    glutMainLoop()
