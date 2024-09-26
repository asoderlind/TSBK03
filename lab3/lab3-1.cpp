// Laboration i spelfysik: Biljardbordet
// By Ingemar Ragnemalm 2010, based on material by Tomas Szabo.
// 2012: Ported to OpenGL 3.2 by Justina Mickonyte and Ingemar R.
// 2013: Adapted to VectorUtils3 and MicroGlut. Variant without zpr
// 2020: Cleanup.
// 2023: Better C++

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define MAIN
#include "GL_utilities.h"
#include "LittleOBJLoader.h"
#include "LoadTGA.h"
#include "MicroGlut.h"
#include "VectorUtils4.h"
// uses framework Cocoa
// uses framework OpenGL

// initial width and heights
const int initWidth = 800, initHeight = 800;

#define NEAR 1.0
#define FAR 100.0

#define kBallSize 0.1  // Radius of a ball

#define abs(x) (x > 0.0 ? x : -x)

typedef struct {
  Model* model;
  GLuint textureId;
} ModelTexturePair;

typedef struct {
  GLuint tex;
  GLfloat mass;

  vec3 X, P, L;  // position, linear momentum, angular momentum
  mat4 R;        // Rotation

  vec3 F, T;  // accumulated force and torque

  //  mat4 J, Ji; We could have these but we can live without them for spheres.
  vec3 omega;  // Angular velocity
  vec3 v;      // Change in velocity

} Ball;

typedef struct {
  GLfloat diffColor[4], specColor[4], ka, kd, ks, shininess;  // coefficients and specular exponent
} Material;

Material ballMt = {{1.0, 1.0, 1.0, 1.0}, {1.0, 1.0, 1.0, 0.0}, 0.1, 0.6, 1.0, 50},
         shadowMt = {{0.0, 0.0, 0.0, 0.5}, {0.0, 0.0, 0.0, 0.5}, 0.1, 0.6, 1.0, 5.0},
         tableMt = {{0.2, 0.1, 0.0, 1.0}, {0.4, 0.2, 0.1, 0.0}, 0.1, 0.6, 1.0, 5.0},
         tableSurfaceMt = {{0.1, 0.5, 0.1, 1.0}, {0.0, 0.0, 0.0, 0.0}, 0.1, 0.6, 1.0, 0.0};

enum { kNumBalls = 16 };  // Change as desired, max 16

//------------------------------Globals---------------------------------
ModelTexturePair tableAndLegs, tableSurf;
Model* sphere;
Ball ball[16];  // We only use kNumBalls but textures for all 16 are always loaded so they must exist. So don't change
                // here, change above.

GLfloat deltaT, currentTime;

vec3 cam, point;

GLuint shader = 0;
GLint lastw = initWidth, lasth = initHeight;  // for resizing
//-----------------------------matrices------------------------------
mat4 projectionMatrix, modelToWorldMatrix, viewMatrix, rotateMatrix, scaleMatrix, transMatrix, tmpMatrix;

//------------------------- lighting--------------------------------
vec3 lightSourcesColorArr[] = {vec3(1.0f, 1.0f, 1.0f)};  // White light
GLfloat specularExponent[] = {50.0};
GLint directional[] = {0};
vec3 lightSourcesDirectionsPositions[] = {vec3(0.0, 10.0, 0.0)};

//----------------------------------Utility functions-----------------------------------

void loadModelTexturePair(ModelTexturePair* modelTexturePair, const char* model, const char* texture) {
  modelTexturePair->model = LoadModelPlus(model);  // , shader, "in_Position", "in_Normal", "in_TexCoord");
  if (texture)
    LoadTGATextureSimple((char*)texture, &modelTexturePair->textureId);
  else
    modelTexturePair->textureId = 0;
}

void renderModelTexturePair(ModelTexturePair* modelTexturePair) {
  if (modelTexturePair->textureId)
    glUniform1i(glGetUniformLocation(shader, "objID"), 0);  // use texture
  else
    glUniform1i(glGetUniformLocation(shader, "objID"), 1);  // use material color only

  glBindTexture(GL_TEXTURE_2D, modelTexturePair->textureId);
  glUniform1i(glGetUniformLocation(shader, "texUnit"), 0);

  DrawModel(modelTexturePair->model, shader, "in_Position", "in_Normal", NULL);
}

void loadMaterial(Material mt) {
  glUniform4fv(glGetUniformLocation(shader, "diffColor"), 1, &mt.diffColor[0]);
  glUniform1fv(glGetUniformLocation(shader, "shininess"), 1, &mt.shininess);
}

//---------------------------------- physics update and billiard table rendering ----------------------------------
void updateWorld() {
  // Zero forces
  int i, j;
  for (i = 0; i < kNumBalls; i++) {
    ball[i].F = SetVector(0, 0, 0);
    ball[i].T = SetVector(0, 0, 0);
  }

  // Wall tests
  for (i = 0; i < kNumBalls; i++) {
    if (ball[i].X.x < -0.82266270 + kBallSize) ball[i].P.x = abs(ball[i].P.x);
    if (ball[i].X.x > 0.82266270 - kBallSize) ball[i].P.x = -abs(ball[i].P.x);
    if (ball[i].X.z < -1.84146270 + kBallSize) ball[i].P.z = abs(ball[i].P.z);
    if (ball[i].X.z > 1.84146270 - kBallSize) ball[i].P.z = -abs(ball[i].P.z);
  }

  // Detect collisions, calculate speed differences, apply forces (uppgift 2)
  for (i = 0; i < kNumBalls; i++)
    for (j = i + 1; j < kNumBalls; j++) {
      // YOUR CODE HERE
    }

  // Control rotation here to movement only, no friction (uppgift 1)
  for (i = 0; i < kNumBalls; i++) {
    ball[i].omega = cross(ball[i].v, SetVector(0, -5, 0));
  }

  // Control rotation here to reflect
  // friction against floor, simplified as well as more correct (uppgift 3)
  for (i = 0; i < kNumBalls; i++) {
    // YOUR CODE HERE
  }

  // Update state, follows the book closely
  for (i = 0; i < kNumBalls; i++) {
    vec3 dX, dP, dL, dO;
    mat4 Rd;

    // Note: omega is not set. How do you calculate it? (del av uppgift 2)
    // YOUR CODE HERE

    //		v := P * 1/mass
    ball[i].v = ball[i].P * 1.0 / (ball[i].mass);
    //		X := X + v*dT
    dX = ball[i].v * deltaT;      // dX := v*dT
    ball[i].X = ball[i].X + dX;   // X := X + dX
                                  //		R := R + Rd*dT
    dO = ball[i].omega * deltaT;  // dO := omega*dT
    Rd = CrossMatrix(dO);         // Calc dO, add to R
    Rd = Rd * ball[i].R;          // Rotate the diff (NOTE: This was missing in early versions.)
    ball[i].R = MatrixAdd(ball[i].R, Rd);
    //		P := P + F * dT
    dP = ball[i].F * deltaT;     // dP := F*dT
    ball[i].P = ball[i].P + dP;  // P := P + dP
                                 //		L := L + t * dT
    dL = ball[i].T * deltaT;     // dL := T*dT
    ball[i].L = ball[i].L + dL;  // L := L + dL

    OrthoNormalizeMatrix(&ball[i].R);
  }
}

void renderBall(int ballNr) {
  glBindTexture(GL_TEXTURE_2D, ball[ballNr].tex);

  // Ball with rotation
  transMatrix = T(ball[ballNr].X.x, kBallSize, ball[ballNr].X.z);  // position
  tmpMatrix = modelToWorldMatrix * transMatrix * ball[ballNr].R;   // ball rotation
  glUniformMatrix4fv(glGetUniformLocation(shader, "modelToWorldMatrix"), 1, GL_TRUE, tmpMatrix.m);
  loadMaterial(ballMt);
  DrawModel(sphere, shader, "in_Position", "in_Normal", NULL);

  // Simple shadow
  glBindTexture(GL_TEXTURE_2D, 0);

  tmpMatrix = modelToWorldMatrix * S(1.0, 0.0, 1.0) * transMatrix * ball[ballNr].R;
  glUniformMatrix4fv(glGetUniformLocation(shader, "modelToWorldMatrix"), 1, GL_TRUE, tmpMatrix.m);
  loadMaterial(shadowMt);
  DrawModel(sphere, shader, "in_Position", "in_Normal", NULL);
}

void renderTable() {
  // Frame and legs, brown, no texture
  loadMaterial(tableMt);
  printError("loading material");
  renderModelTexturePair(&tableAndLegs);

  // Table surface (green texture)
  loadMaterial(tableSurfaceMt);
  renderModelTexturePair(&tableSurf);
}
//-------------------------------------------------------------------------------------

void init() {
  dumpInfo();  // shader info

  // GL inits
  glClearDepth(1.0);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  printError("GL inits");

  // Load shader
  shader = loadShaders("lab3.vert", "lab3.frag");
  printError("init shader");

  loadModelTexturePair(&tableAndLegs, "tableandlegsnosurf.obj", 0);
  loadModelTexturePair(&tableSurf, "tablesurf.obj", "surface.tga");
  sphere = LoadModelPlus("sphere.obj");

  projectionMatrix = perspective(90, 1.0, 0.1, 1000);  // It would be silly to upload an uninitialized matrix
  glUniformMatrix4fv(glGetUniformLocation(shader, "projMatrix"), 1, GL_TRUE, projectionMatrix.m);

  modelToWorldMatrix = IdentityMatrix();

  char* textureStr = (char*)malloc(128);
  int i;
  for (i = 0; i < kNumBalls; i++) {
    sprintf(textureStr, "balls/%d.tga", i);
    LoadTGATextureSimple(textureStr, &ball[i].tex);
  }
  free(textureStr);

  // Initialize ball data, positions etc
  for (i = 0; i < kNumBalls; i++) {
    ball[i].mass = 1.0;
    ball[i].X = vec3(0.0, 0.0, 0.0);
    ball[i].P = vec3(((float)(i % 13)) / 50.0, 0.0, ((float)(i % 15)) / 50.0);
    ball[i].R = IdentityMatrix();
  }
  ball[0].X = vec3(0, 0, 0);
  ball[1].X = vec3(0, 0, 0.5);
  ball[2].X = vec3(0.0, 0, 1.0);
  ball[3].X = vec3(0, 0, 1.5);
  ball[0].P = vec3(0, 0, 0);
  ball[1].P = vec3(0, 0, 0);
  ball[2].P = vec3(0, 0, 0);
  ball[3].P = vec3(0, 0, 1.00);

  cam = vec3(0, 1.2, 2.5);
  point = vec3(0, 0, 1.0);
  viewMatrix = lookAtv(cam, point, vec3(0, 1, 0));
}

//-------------------------------callback functions------------------------------------------
void display(void) {
  deltaT = glutGet(GLUT_ELAPSED_TIME) / 1000.0 - currentTime;
  currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0;

  int i;
  // This function is called whenever it is time to render
  //  a new frame; due to the idle()-function below, this
  //  function will get called several times per second
  updateWorld();

  // Clear framebuffer & zbuffer
  glClearColor(0.4, 0.5, 0.9, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  glUniformMatrix4fv(glGetUniformLocation(shader, "viewMatrix"), 1, GL_TRUE, viewMatrix.m);
  glUniformMatrix4fv(glGetUniformLocation(shader, "modelToWorldMatrix"), 1, GL_TRUE, modelToWorldMatrix.m);

  printError("uploading to shader");

  renderTable();

  for (i = 0; i < kNumBalls; i++) renderBall(i);

  printError("rendering");

  glutSwapBuffers();
}

// Trackball
// This is ONLY for making it possible to rotate the table in order to inspect it.

int prevx = 0, prevy = 0;

void mouseUpDown(int button, int state, int x, int y) {
  if (state == GLUT_DOWN) {
    prevx = x;
    prevy = y;
  }
}

void mouseDragged(int x, int y) {
  vec3 p;
  mat4 m;

  // This is a simple and IMHO really nice trackball system:

  // Use the movement direction to create an orthogonal rotation axis

  p.y = x - prevx;
  p.x = -(prevy - y);
  p.z = 0;

  // Create a rotation around this axis and premultiply it on the model-to-world matrix
  // Limited to fixed camera! Will be wrong if the camera is moved!

  m = ArbRotate(p, sqrt(p.x * p.x + p.y * p.y) / 250.0);  // Rotation in view coordinates
  modelToWorldMatrix = m * modelToWorldMatrix;

  prevx = x;
  prevy = y;

  glutPostRedisplay();
}

void reshape(GLsizei w, GLsizei h) {
  lastw = w;
  lasth = h;

  glViewport(0, 0, w, h);
  GLfloat ratio = (GLfloat)w / (GLfloat)h;
  projectionMatrix = perspective(90, ratio, 0.1, 1000);
  glUniformMatrix4fv(glGetUniformLocation(shader, "projMatrix"), 1, GL_TRUE, projectionMatrix.m);
}

//-----------------------------main-----------------------------------------------
int main(int argc, char** argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
  glutInitWindowSize(initWidth, initHeight);
  glutInitContextVersion(3, 2);
  glutCreateWindow("Biljardbordet");
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);

  glutMouseFunc(mouseUpDown);
  glutMotionFunc(mouseDragged);

  glutRepeatingTimer(20);

  init();

  glutMainLoop();
  exit(0);
}
