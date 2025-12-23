#define STB_IMAGE_IMPLEMENTATION
#include <cmath>
#include <vector>
#include <iostream>

#include <GLFW/glfw3.h>
#include "stb_image.h"
#include <corecrt_math_defines.h>

using namespace std;

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

float camDist = 14.0f;
float camRotX = 20.0f;
float camRotY = -30.0f;

bool mousePressed = false;
double lastX, lastY;

GLuint cubeTexture;
GLuint sphereTexture;
GLuint prismTexture;

enum ShapeType { 
    CUBE, SPHERE, TRI_PRISM 
};

struct Object3D {
    ShapeType type;
    float x, y, z;
    float rot;
    float radius;
};

vector<Object3D> objects;
int selected = -1;

void perspective(float fovY, float aspect, float zNear, float zFar) {
    float fH = tanf(fovY * M_PI / 360.0f) * zNear;
    float fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}

void setupLight() {
    glEnable(GL_LIGHTING);

    GLfloat pos0[] = {5, 5, 5, 1};
    GLfloat col0[] = {1, 1, 1, 1};
    GLfloat amb0[] = {0.3f, 0.3f, 0.3f, 1};

    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, pos0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, col0);
    glLightfv(GL_LIGHT0, GL_AMBIENT, amb0);

    GLfloat pos1[] = {0, 0, 0, 1};
    GLfloat col1[] = {0.8f, 0.8f, 0.8f, 1};

    glEnable(GL_LIGHT1);
    glLightfv(GL_LIGHT1, GL_POSITION, pos1);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, col1);
}

GLuint loadTexture(const char* path) {
    int w, h, ch;
    unsigned char* data = stbi_load(path, &w, &h, &ch, 0);
    if (!data) {
        cout << "Failed to load texture\n";
        return 0;
    }

    GLenum format = (ch == 4) ? GL_RGBA : GL_RGB;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0,
        format, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    return tex;
}

void applyHighlight(bool active) {
    if (active) {
        GLfloat emissive[] = {0.4f, 0.4f, 0.0f, 1.0f};
        glMaterialfv(GL_FRONT, GL_EMISSION, emissive);
    }
    else {
        GLfloat emissive[] = {0, 0, 0, 1};
        glMaterialfv(GL_FRONT, GL_EMISSION, emissive);
    }
}

void drawCube() {
    glBegin(GL_QUADS);

    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0); glVertex3f(-1, -1, 1);
    glTexCoord2f(1, 0); glVertex3f(1, -1, 1);
    glTexCoord2f(1, 1); glVertex3f(1, 1, 1);
    glTexCoord2f(0, 1); glVertex3f(-1, 1, 1);

    glNormal3f(0, 0, -1);
    glTexCoord2f(0, 0); glVertex3f(1, -1, -1);
    glTexCoord2f(1, 0); glVertex3f(-1, -1, -1);
    glTexCoord2f(1, 1); glVertex3f(-1, 1, -1);
    glTexCoord2f(0, 1); glVertex3f(1, 1, -1);

    glNormal3f(-1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
    glTexCoord2f(1, 0); glVertex3f(-1, -1, 1);
    glTexCoord2f(1, 1); glVertex3f(-1, 1, 1);
    glTexCoord2f(0, 1); glVertex3f(-1, 1, -1);

    glNormal3f(1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(1, -1, 1);
    glTexCoord2f(1, 0); glVertex3f(1, -1, -1);
    glTexCoord2f(1, 1); glVertex3f(1, 1, -1);
    glTexCoord2f(0, 1); glVertex3f(1, 1, 1);

    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0); glVertex3f(-1, 1, 1);
    glTexCoord2f(1, 0); glVertex3f(1, 1, 1);
    glTexCoord2f(1, 1); glVertex3f(1, 1, -1);
    glTexCoord2f(0, 1); glVertex3f(-1, 1, -1);

    glNormal3f(0, -1, 0);
    glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
    glTexCoord2f(1, 0); glVertex3f(1, -1, -1);
    glTexCoord2f(1, 1); glVertex3f(1, -1, 1);
    glTexCoord2f(0, 1); glVertex3f(-1, -1, 1);

    glEnd();
}

void drawSphere(float r) {
    for (int i = 0; i < 24; i++) {
        float lat0 = M_PI * (-0.5f + i / 24.0f);
        float lat1 = M_PI * (-0.5f + (i + 1) / 24.0f);

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= 24; j++) {
            float lng = 2 * M_PI * j / 24;
            float x = cos(lng);
            float y = sin(lng);

            float u = j / 24.0f;
            float v0 = i / 24.0f;
            float v1 = (i + 1) / 24.0f;

            glNormal3f(x * cos(lat0), y * cos(lat0), sin(lat0));
            glTexCoord2f(u, v0);
            glVertex3f(r * x * cos(lat0), r * y * cos(lat0), r * sin(lat0));

            glNormal3f(x * cos(lat1), y * cos(lat1), sin(lat1));
            glTexCoord2f(u, v1);
            glVertex3f(r * x * cos(lat1), r * y * cos(lat1), r * sin(lat1));
        }
        glEnd();
    }
}

void drawTriPrism() {
    glBegin(GL_TRIANGLES);
    glNormal3f(0, 0, 1);
    glTexCoord2f(0.5f, 1.0f); glVertex3f(0, 1, 1);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-1, -1, 1);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(1, -1, 1);
    glEnd();

    glBegin(GL_TRIANGLES);
    glNormal3f(0, 0, -1);
    glTexCoord2f(0.5f, 1.0f); glVertex3f(0, 1, -1);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(1, -1, -1);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-1, -1, -1);
    glEnd();

    glBegin(GL_QUADS);

    glNormal3f(-1, 0, 0);
    glTexCoord2f(0, 1); glVertex3f(0, 1, 1);
    glTexCoord2f(1, 1); glVertex3f(0, 1, -1);
    glTexCoord2f(1, 0); glVertex3f(-1, -1, -1);
    glTexCoord2f(0, 0); glVertex3f(-1, -1, 1);

    glNormal3f(1, 0, 0);
    glTexCoord2f(0, 1); glVertex3f(0, 1, 1);
    glTexCoord2f(1, 1); glVertex3f(1, -1, 1);
    glTexCoord2f(1, 0); glVertex3f(1, -1, -1);
    glTexCoord2f(0, 0); glVertex3f(0, 1, -1);

    glNormal3f(0, -1, 0);
    glTexCoord2f(0, 1); glVertex3f(-1, -1, 1);
    glTexCoord2f(1, 1); glVertex3f(-1, -1, -1);
    glTexCoord2f(1, 0); glVertex3f(1, -1, -1);
    glTexCoord2f(0, 0); glVertex3f(1, -1, 1);

    glEnd();
}

void drawOutline(void (*drawFunc)()) {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2.5f);
    glColor3f(1, 1, 0);
    drawFunc();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

void worldToCamera(float wx, float wy, float wz,
    float& cx, float& cy) {

    float ry = camRotY * M_PI / 180.0f;
    float rx = camRotX * M_PI / 180.0f;

    float x1 = cos(ry) * wx + sin(ry) * wz;
    float z1 = -sin(ry) * wx + cos(ry) * wz;

    float y1 = cos(rx) * wy - sin(rx) * z1;

    cx = x1;
    cy = y1;
}

void mouseButton(GLFWwindow* w, int btn, int act, int) {
    if (btn == GLFW_MOUSE_BUTTON_LEFT) {
        if (act == GLFW_PRESS) {
            mousePressed = true;
            glfwGetCursorPos(w, &lastX, &lastY);
            double x, y; glfwGetCursorPos(w, &x, &y);
            float nx = (x / SCREEN_WIDTH - 0.5f) * 20;
            float ny = -(y / SCREEN_HEIGHT - 0.5f) * 20;
            selected = -1;
            for (int i = 0;i < objects.size();i++) {
                float cx, cy;
                worldToCamera(objects[i].x, objects[i].y, objects[i].z, cx, cy);

                if (fabs(nx - cx) < objects[i].radius && fabs(ny - cy) < objects[i].radius) {
                    selected = i;
                }
            }
                
        }
        else mousePressed = false;
    }
}

void mouseMove(GLFWwindow*, double x, double y) {
    if (!mousePressed) return;
    camRotY += (x - lastX) * 0.3f;
    camRotX += (y - lastY) * 0.3f;
    lastX = x; lastY = y;
}

void scroll(GLFWwindow*, double, double y) {
    camDist -= y;
    if (camDist < 6) camDist = 6;
}

void keyboard(GLFWwindow* w) {
    if (selected < 0) return;
    auto& o = objects[selected];
    if (glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS) o.x -= 0.005f;
    if (glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS) o.x += 0.005f;
    if (glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS) o.y += 0.005f;
    if (glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS) o.y -= 0.005f;
    if (glfwGetKey(w, GLFW_KEY_R) == GLFW_PRESS) o.rot -= 0.05f;
    if (glfwGetKey(w, GLFW_KEY_T) == GLFW_PRESS) o.rot += 0.05f;
}

int main() {
    glfwInit();
    GLFWwindow* w = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "3D Objects", 0, 0);
    glfwMakeContextCurrent(w);

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    glfwSetMouseButtonCallback(w, mouseButton);
    glfwSetCursorPosCallback(w, mouseMove);
    glfwSetScrollCallback(w, scroll);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    setupLight();
    cubeTexture = loadTexture("crate.jpg");
    sphereTexture = loadTexture("metal.jpg");
    prismTexture = loadTexture("brick.jpg");

    objects.push_back({CUBE, -4, 0, 0, 0, 1.5f});
    objects.push_back({TRI_PRISM, 0, 0, 0, 0, 1.5f});
    objects.push_back({SPHERE, 4, 0, 0, 0, 1.5f});

    while (!glfwWindowShouldClose(w)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        perspective(45, (float)SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 100);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0, 0, -camDist);
        glRotatef(camRotX, 1, 0, 0);
        glRotatef(camRotY, 0, 1, 0);

        keyboard(w);

        for (int i = 0;i < objects.size();i++) {
            bool sel = (i == selected);

            glPushMatrix();
            glTranslatef(objects[i].x, objects[i].y, objects[i].z);
            glRotatef(objects[i].rot, 0, 1, 0);
            applyHighlight(sel);

            if (objects[i].type == CUBE) {
				glBindTexture(GL_TEXTURE_2D, cubeTexture);
                drawCube();
                if (sel) drawOutline(drawCube);
            }
            if (objects[i].type == SPHERE) {
                glBindTexture(GL_TEXTURE_2D, sphereTexture);
                drawSphere(1);
                if (sel) drawOutline([]() { drawSphere(1); });
            }
            if (objects[i].type == TRI_PRISM) {
                glBindTexture(GL_TEXTURE_2D, prismTexture);
                drawTriPrism();
                if (sel) drawOutline(drawTriPrism);
            }
            applyHighlight(false);
            glPopMatrix();
        }

        glfwSwapBuffers(w);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}