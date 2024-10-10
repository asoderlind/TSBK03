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

float ballDistance(Ball& a, Ball& b) {
  return sqrt(pow(a.X.x - b.X.x, 2) + pow(a.X.y - b.X.y, 2) + pow(a.X.z - b.X.z, 2));
}

bool isColliding(Ball& a, Ball& b) { return ballDistance(a, b) < 2 * kBallSize; }

//---------------------------------- physics update and billiard table rendering ----------------------------------

mat4 calculateInertiaTensor(float mass, float radius) {
  float I_element = (2.f / 5.f) * mass * pow(radius, 2);
  mat4 I = IdentityMatrix();
  I.m[0] = I_element;
  I.m[5] = I_element;
  I.m[10] = I_element;
  return I;
}

vec3 calculateOmega(const mat4& I, const vec3& L) {
  mat4 I_inv = InvertMat4(I);
  return I_inv * L;
}

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
      if (isColliding(ball[i], ball[j])) {
        // printf("Collision between ball %d and ball %d\n", i, j);
        // vec3 nonNormalizedDirIntersection = ball[j].X - ball[i].X;
        // vec3 dirIntersection;
        // float absIntersection;
        // if (nonNormalizedDirIntersection.x == 0 && nonNormalizedDirIntersection.y == 0 &&
        //     nonNormalizedDirIntersection.z == 0) {
        //   // the balls are completely on top of each other
        //   // push out in random direction
        //   dirIntersection = vec3(rand() % 100, 0, rand() % 100);
        //   absIntersection = 2 * kBallSize * 0.00001;
        // } else {
        //   dirIntersection = normalize(ball[j].X - ball[i].X);
        //   absIntersection = (2 * kBallSize - ballDistance(ball[i], ball[j])) * 0.7;
        // }
        // ball[i].X -= 0.5 * dirIntersection * absIntersection;
        // ball[j].X += 0.5 * dirIntersection * absIntersection;

        // relative position
        vec3 r_A = (ball[j].X - ball[i].X) / 2.f;
        vec3 r_B = (ball[i].X - ball[j].X) / 2.f;
        vec3 n = normalize(r_B);  // normal vector

        // initial angular velocities
        vec3 omega_a_before = ball[i].omega;
        vec3 omega_b_before = ball[j].omega;

        // initial velocities
        vec3 v_a_before = ball[i].v;
        vec3 v_b_before = ball[j].v;

        // relative velocity
        vec3 v_p_A = v_a_before + cross(omega_a_before, r_A);
        vec3 v_p_B = v_b_before + cross(omega_b_before, r_B);
        vec3 v_rel = v_p_A - v_p_B;
        GLfloat v_rel_before = dot(v_rel, n);

        mat4 I_A = calculateInertiaTensor(ball[i].mass, kBallSize);
        mat4 I_B = calculateInertiaTensor(ball[j].mass, kBallSize);

        float restitution = 1.0;
        // Testfall 3
        // restitution = 0.5;
        // restitution = 0.0;

        float numerator = -(1 + restitution) * v_rel_before;
        /* float denominator =
            2 / ball[i].mass + dot(n, I_inv * cross(cross(r_A, n), r_A)) + dot(n, I_inv * cross(cross(r_B, n), r_B)); */
        float denominator = 1 / ball[i].mass + 1 / ball[j].mass;
        float j_imp = numerator / denominator;

        vec3 impulse = j_imp * n;

        // apply impulse
        ball[i].P += impulse;
        ball[j].P -= impulse;

        // update angular velocities
        // ball[i].omega = omega_a_before + j_imp * inverse(I_A) * cross(r_A, n);
        // ball[j].omega = omega_b_before - j_imp * inverse(I_B) * cross(r_B, n);

        // update torque
        ball[i].T += cross(r_A, impulse);
        ball[j].T -= cross(r_B, impulse);
      }
    }

  // Control rotation here to reflect
  // friction against floor, simplified as well as more correct (uppgift 3)
  for (i = 0; i < kNumBalls; i++) {
  }

  // Update state, follows the book closely
  for (i = 0; i < kNumBalls; i++) {
    vec3 dX, dP, dL, dO;
    mat4 Rd;

    // Note: omega is not set. How do you calculate it? (del av uppgift 2)
    //		omega := I^-1 * L
    // float I = 2.0 / 5.0 * ball[i].mass * kBallSize * kBallSize;
    // ball[i].omega = (1.f / I) * ball[i].L;
    // ball[i].omega = cross(ball[i].v, SetVector(0, -5, 0));

    //		v := P * 1/mass
    // 1. Calculate current velocity and angular velocity
    ball[i].v = ball[i].P * 1.0 / (ball[i].mass);
    // printf("v: %f %f %f\n", ball[i].v.x, ball[i].v.y, ball[i].v.z);
    mat4 I = calculateInertiaTensor(ball[i].mass, kBallSize);
    ball[i].omega = calculateOmega(I, ball[i].L);
    // printf("omega: %f %f %f\n", ball[i].omega.x, ball[i].omega.y, ball[i].omega.z);

    // 2. Calculate force against the ground
    vec3 contactPoint = SetVector(ball[i].X.x, -kBallSize, ball[i].X.z);
    // printf("contactPoint: %f %f %f\n", contactPoint.x, contactPoint.y, contactPoint.z);
    vec3 r = contactPoint - ball[i].X;
    // printf("r: %f %f %f\n", r.x, r.y, r.z);
    vec3 v_contact = ball[i].v + cross(ball[i].omega, r);
    // printf("v_contact: %f %f %f\n", v_contact.x, v_contact.y, v_contact.z);

    float friction_coefficient = 0.1;  // Adjust as needed
    float g = 9.81;
    vec3 friction_force = SetVector(0, 0, 0);
    if (v_contact.x != 0 || v_contact.y != 0 || v_contact.z != 0) {
      friction_force = -friction_coefficient * ball[i].mass * g * normalize(v_contact);
    }
    // printf("v_contact: %f %f %f\n", v_contact.x, v_contact.y, v_contact.z);
    ball[i].F += friction_force;

    // 3. Calculate torque from friction force
    ball[i].T = cross(r, friction_force);
    // printf("T: %f %f %f\n", ball[i].T.x, ball[i].T.y, ball[i].T.z);

    // printf("L: %f %f %f\n", ball[i].L.x, ball[i].L.y, ball[i].L.z);
    // printf("omega: %f %f %f\n", ball[i].omega.x, ball[i].omega.y, ball[i].omega.z);
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
    // ball[i].P = vec3(0.0, 0.0, 0.0);
    printf("P: %f %f %f\n", ball[i].P.x, ball[i].P.y, ball[i].P.z);
    ball[i].R = IdentityMatrix();
    ball[i].L = SetVector(0, 0, 0);
    ball[i].F = SetVector(0, 0, 0);
    ball[i].T = SetVector(0, 0, 0);
    ball[i].omega = SetVector(0, 0, 0);
    ball[i].v = SetVector(0, 0, 0);
    ball[i].P = SetVector(0, 0, 0);
  }

  ball[0].X = vec3(0, 0, 0);
  ball[1].X = vec3(0, 0, 0.5);
  ball[2].X = vec3(0.0, 0, 1.0);
  ball[3].X = vec3(0, 0, 1.5);
  // ball[0].P = vec3(0, 0, 0);
  // ball[1].P = vec3(0, 0, 0);
  // ball[2].P = vec3(0, 0, -1);
  ball[3].P = vec3(0, 0, -2.00);
  // ball[3].P = vec3(0, 0, 0.0);

  // for (i = 4; i < kNumBalls; i++) {
  //   // add small position offsets to avoid initial collisions
  //   ball[i].X = vec3(0, 0, 0.5 + 0.001 * i);
  // }

  // Testfall 2
  // ball[0].mass = 5.0;
  // ball[0].X = vec3(0.5, 0.0, 0.0);
  // ball[1].X = vec3(0.5, 0.0, 0.5);
  // ball[3].P = vec3(0.5, 0, -1.00);

  cam = vec3(0, 1.2, 1.5);
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
