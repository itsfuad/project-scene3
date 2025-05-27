#include <GL/glut.h>
#include <GL/gl.h>
#include <cmath>
#include <stdio.h>

// Define the traffic light states and cycle order.
enum class TRAFFIC_LIGHT_STATE {
    RED,
    YELLOW,
    GREEN
};

TRAFFIC_LIGHT_STATE currentTrafficLightState = TRAFFIC_LIGHT_STATE::RED;
int cycleStep = 0; // cycles: 0=RED, 1=YELLOW, 2=GREEN, 3=YELLOW

void drawRect(float x, float y, float width, float height) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

void drawLine(float x1, float y1, float x2, float y2, float thickness = 1.0f) {
    glLineWidth(thickness);
    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
}

void drawCircle(float cx, float cy, float r, int startAngle = 0, int endAngle = 360, int num_segments = 30) {
    glBegin(GL_POLYGON);
    for (int ii = startAngle; ii < endAngle; ii++) {
        float theta = 2.0f * 3.1415926f * float(ii) / float(num_segments);
        float circ_x = r * cosf(theta);
        float circ_y = r * sinf(theta);
        glVertex2f(circ_x + cx, circ_y + cy);
    }
    glEnd();
}

void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3) {
    glBegin(GL_TRIANGLES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glVertex2f(x3, y3);
    glEnd();
}

void drawHumanShape(float x, float y, float scale, int walkState) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);
    
    if (walkState == 1) {
        // WALK state - walking figure
        //color 
        glColor3f(0.0f, 0.9f, 0.0f); // Green for "WALK"
        //left hand
        drawLine(-7, -4, -3, 4, 2.0f);
        drawLine(6, 0, 3, 4, 2.0f);
        drawLine(6, -4, 6, 0, 2.0f);
        //legs as ^
        drawLine(-5, -10, -1, -2, 2.0f);
        drawLine(5, -10, 1, -2, 2.0f);
    } else {
        // STOP state - standing figure
        glColor3f(0.9f, 0.0f, 0.0f); // Red for "STOP"
        //left hand
        drawLine(-3, -4, -3, 4, 2.0f);
        //right hand
        drawLine(3.2, -4, 3.2, 4, 2.0f);
        //left leg
        drawLine(-1, -10, -1, -2, 2.0f);
        //right leg
        drawLine(1.2, -10, 1.2, -2, 2.0f);
    }

    drawCircle(0, 7.5, 3);
    drawRect(-2, -4, 4, 8); // Body
    
    glPopMatrix();
}

void drawTrafficSignal(float x, float y, TRAFFIC_LIGHT_STATE state) {
    const float height = 200.0f;
    const float width = 70.0f;
    const float lightRadius = 25.0f;
    float spacing = height / 3.0f;
    int translateY = 25; // constant offset for bulbs

    // Draw three segments (top, middle, bottom)
    for (int i = 0; i < 3; i++) {
        float centerY = y + height - (i + 0.5f) * spacing;
        // Determine bulb color based on segment and current state.
        if(i == 0 && state == TRAFFIC_LIGHT_STATE::RED)
            glColor3f(1.0f, 0.0f, 0.0f);
        else if(i == 1 && state == TRAFFIC_LIGHT_STATE::YELLOW)
            glColor3f(1.0f, 1.0f, 0.0f);
        else if(i == 2 && state == TRAFFIC_LIGHT_STATE::GREEN)
            glColor3f(0.0f, 1.0f, 0.0f);
        else
            glColor3f(0.3f, 0.3f, 0.3f); // dim bulb when off

        // Draw left and right semi-circles for the bulb.
        drawCircle(x + 15, centerY + translateY - lightRadius + 2, lightRadius, 0, 180, 30);
        drawCircle(x + width - 15, centerY + translateY - lightRadius + 2, lightRadius, 0, 180, 30);

        // Draw triangle shapes on each side (using fixed black color).
        glColor3f(0.0f, 0.0f, 0.0f);
        drawTriangle(x, centerY + translateY, x - 20, centerY + translateY, x, centerY - 10 + translateY);
        drawTriangle(x + width, centerY + translateY, x + width + 20, centerY + translateY, x + width, centerY - 10 + translateY);
    }
    // Draw the main black signal box.
    glColor3f(0.0f, 0.0f, 0.0f);
    drawRect(x, y, width, height);
}

void drawText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* p = text; *p; p++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
    }
}

void drawHumanSign(float x, float y, int walkState) {

    if (walkState == 1)
        glColor3f(0.0f, 1.0f, 0.0f); // Cyan for "WALK"
    else
        glColor3f(0.9f, 0.0f, 0.0f);

    drawHumanShape(x, y, 3, walkState);

    // Draw text below
    const char* msg = (walkState == 1) ? "WALK" : "STOP";
    drawText(x - 25, y - 55, msg);
}

void display() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 800, 0.0, 600);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    drawTrafficSignal(100, 100, currentTrafficLightState);
    // Draw human sign on top of the traffic signal's rectangle.
    // Traffic signal is drawn at (100,100) with width 70, so center approx at x=135.
    // We choose a y coordinate (e.g. 150) so it sits inside the signal.
    int humanState = (currentTrafficLightState == TRAFFIC_LIGHT_STATE::GREEN) ? 0 : 1;
    drawHumanSign(100 + 70/2.0f, 200, humanState);
    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 't' || key == 'T') {
        // Cycle through: RED -> YELLOW -> GREEN -> YELLOW -> RED
        cycleStep = (cycleStep + 1) % 4;
        switch(cycleStep) {
            case 0: currentTrafficLightState = TRAFFIC_LIGHT_STATE::RED; break;
            case 1: currentTrafficLightState = TRAFFIC_LIGHT_STATE::YELLOW; break;
            case 2: currentTrafficLightState = TRAFFIC_LIGHT_STATE::GREEN; break;
            case 3: currentTrafficLightState = TRAFFIC_LIGHT_STATE::YELLOW; break;
        }
        glutPostRedisplay();
    }
    
}

int main(int argc, char* argv[]) {
    glutInit(&argc, argv); // initialize GLUT
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("OpenGL Drawing Functions");
    glutDisplayFunc(display); // register display callback
    glutKeyboardFunc(keyboard); // register keyboard callback
    glutMainLoop(); // enter main loop
    return 0;
}