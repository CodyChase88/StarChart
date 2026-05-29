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

window_width = 800
window_height = 600
hover_threshold = 10  # pixels

stars = []
mouse_pos = (0, 0)

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

            # Convert RA/Dec to degrees
            ra_deg = (ra_h + ra_m/60 + ra_s/3600) * 15.0
            dec_deg = dec_d + (dec_m/60 if dec_d >= 0 else -dec_m/60) + (dec_s/3600 if dec_d >= 0 else -dec_s/3600)
            
            # Convert to radians
            ra_rad = math.radians(ra_deg)
            dec_rad = math.radians(dec_deg)
            
            # Convert spherical to Cartesian coordinates
            x = dist_ly * math.cos(dec_rad) * math.cos(ra_rad)
            y = dist_ly * math.cos(dec_rad) * math.sin(ra_rad)
            z = dist_ly * math.sin(dec_rad)

            stars.append({"pos": (x, y, z), "name": name, "dist": dist_ly})

# ---- Determine color based on distance ----
def star_color(star):
    if star["name"].lower() == "sol":
        return (1.0, 1.0, 0.9)
    dist = max(star["dist"], 0.01)
    brightness = min(1.0, 10.0 / dist)
    return (brightness, brightness * 0.9, brightness * 0.7)

# ---- Determine size based on distance ----
def star_size(star):
    if star["name"].lower() == "sol":
        return 0.1
    dist = max(star["dist"], 0.01)
    return max(0.05, 0.15 / dist)

# ---- Project 3D point to 2D screen ----
def project_point(x, y, z):
    modelview = glGetDoublev(GL_MODELVIEW_MATRIX)
    projection = glGetDoublev(GL_PROJECTION_MATRIX)
    viewport = glGetIntegerv(GL_VIEWPORT)
    winX, winY, winZ = gluProject(x, y, z, modelview, projection, viewport)
    winY = viewport[3] - winY
    return winX, winY, winZ

# ---- Draw text using GLUT bitmap ----
def draw_text(x, y, text):
    glRasterPos2f(x, y)
    for ch in text:
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, ord(ch))

# ---- OpenGL display ----
def display():
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    glLoadIdentity()
    
    # Camera transform
    glTranslatef(0.0, 0.0, -camera_distance)
    glRotatef(camera_angle_x, 1, 0, 0)
    glRotatef(camera_angle_y, 0, 1, 0)
    
    # Draw stars
    for star in stars:
        x, y, z = star["pos"]
        size = star_size(star)
        color = star_color(star)
        glColor3f(*color)
        glPushMatrix()
        glTranslatef(x, y, z)
        glutSolidSphere(size, 8, 8)
        glPopMatrix()
    
    # Draw name for star under mouse
    nearest_star = None
    min_dist = hover_threshold
    for star in stars:
        sx, sy, sz = star["pos"]
        screen_x, screen_y, _ = project_point(sx, sy, sz)
        dist = math.hypot(mouse_pos[0] - screen_x, mouse_pos[1] - screen_y)
        if dist < min_dist:
            nearest_star = star
            min_dist = dist

    if nearest_star:
        glMatrixMode(GL_PROJECTION)
        glPushMatrix()
        glLoadIdentity()
        gluOrtho2D(0, window_width, 0, window_height)
        glMatrixMode(GL_MODELVIEW)
        glPushMatrix()
        glLoadIdentity()
        glColor3f(1.0, 1.0, 0.0)
        draw_text(mouse_pos[0] + 10, window_height - mouse_pos[1] - 10, nearest_star["name"])
        glPopMatrix()
        glMatrixMode(GL_PROJECTION)
        glPopMatrix()
        glMatrixMode(GL_MODELVIEW)
    
    glutSwapBuffers()

# ---- Window reshape ----
def reshape(width, height):
    global window_width, window_height
    window_width = width
    window_height = height
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
    update_mouse(x, y)

def update_mouse(x, y):
    global mouse_pos
    mouse_pos = (x, y)
    glutPostRedisplay()

# ---- Mouse wheel for zoom ----
def mouse_wheel(button, dir, x, y):
    global camera_distance
    if dir > 0:
        camera_distance *= 0.9
    else:
        camera_distance *= 1.1
    glutPostRedisplay()

# ---- Initialization ----
def init():
    glEnable(GL_DEPTH_TEST)
    glClearColor(0, 0, 0, 1)

# ---- Main ----
if __name__ == "__main__":
    load_stars("data/stars_query.csv")
    
    glutInit()
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH)
    glutInitWindowSize(window_width, window_height)
    glutCreateWindow(b"3D Star Map")
    
    init()
    
    glutDisplayFunc(display)
    glutReshapeFunc(reshape)
    glutMouseFunc(mouse)
    glutMotionFunc(motion)
    glutPassiveMotionFunc(update_mouse)
    
    try:
        glutMouseWheelFunc(mouse_wheel)
    except:
        print("Mouse wheel may not be supported; zoom manually by changing camera_distance")
    
    glutMainLoop()
