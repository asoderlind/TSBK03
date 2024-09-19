// Part B: Many-bone worm

// 2022: C++ version.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define MAIN
#include "GL_utilities.h"
#include "LittleOBJLoader.h"
#include "MicroGlut.h"
#include "VectorUtils4.h"
// uses framework Cocoa
// uses framework OpenGL
#include <string.h>

// Ref till shader
GLuint g_shader;

typedef struct Triangle {
  GLuint v1;
  GLuint v2;
  GLuint v3;
} Triangle;

#define CYLINDER_SEGMENT_LENGTH 0.37
#define kMaxRow 100
#define kMaxCorners 8
#define kMaxBones 10
#define kMaxg_poly ((kMaxRow - 1) * kMaxCorners * 2)
#ifndef Pi
#define Pi 3.1416
#endif
#ifndef true
#define true 1
#endif

#define BONE_LENGTH 4.0

Triangle g_poly[kMaxg_poly];

// vertices
vec3 g_vertsOrg[kMaxRow][kMaxCorners];
vec3 g_normalsOrg[kMaxRow][kMaxCorners];
vec3 g_vertsRes[kMaxRow][kMaxCorners];
vec3 g_normalsRes[kMaxRow][kMaxCorners];

// vertex attributes
float g_boneWeights[kMaxRow][kMaxCorners][kMaxBones];
vec2 g_boneWeightVis[kMaxRow][kMaxCorners];  // Copy data to here to visualize your weights

Model *cylinderModel;  // Collects all the above for drawing with glDrawElements

mat4 modelViewMatrix, projectionMatrix;

mat4 T_bones[kMaxBones], M_i[kMaxBones];

///////////////////////////////////////////////////
//		I N I T  B O N E  W E I G H T S
// Desc:  initierar benvikterna
//
void initBoneWeights(void) {
  long row, corner;
  int bone;

  // Set values for all vertices in the mesh
  for (row = 0; row < kMaxRow; row++)
    for (corner = 0; corner < kMaxCorners; corner++) {
      float boneWeights[kMaxBones];
      float totalBoneWeight = 0.f;

      float maxBoneWeight = 0.f;

      for (bone = 0; bone < kMaxBones; bone++) {
        float bonePos = BONE_LENGTH * bone;
        float boneDist = fabs(bonePos - g_vertsOrg[row][corner].x);
        float boneWeight = (BONE_LENGTH - boneDist) / (BONE_LENGTH);
        if (boneWeight < 0) boneWeight = 0;
        boneWeights[bone] = boneWeight;
        totalBoneWeight += boneWeight;

        if (maxBoneWeight < boneWeight) maxBoneWeight = boneWeight;
      }

      g_boneWeightVis[row][corner].x = 0;
      g_boneWeightVis[row][corner].y = 0;
      for (bone = 0; bone < kMaxBones; bone++) {
        g_boneWeights[row][corner][bone] = boneWeights[bone] / totalBoneWeight;

        if (bone & 1)
          g_boneWeightVis[row][corner].x +=
              g_boneWeights[row][corner][bone];  // Copy data to here to visualize your weights or anything else
        if ((bone + 1) & 1)
          g_boneWeightVis[row][corner].y +=
              g_boneWeights[row][corner][bone];  // Copy data to here to visualize your weightss
      }
    }
}

///////////////////////////////////////////////////
//		B U I L D  C Y L I N D E R
// Desc: Builds the geometry
//
void BuildCylinder() {
  long row, corner, cornerIndex;

  // Sets the values for all vertices in the mesh
  for (row = 0; row < kMaxRow; row++)
    for (corner = 0; corner < kMaxCorners; corner++) {
      g_vertsOrg[row][corner].x = (float)row * CYLINDER_SEGMENT_LENGTH;
      g_vertsOrg[row][corner].y = cos(corner * 2 * Pi / kMaxCorners);
      g_vertsOrg[row][corner].z = sin(corner * 2 * Pi / kMaxCorners);

      g_normalsOrg[row][corner].x = 0;
      g_normalsOrg[row][corner].y = cos(corner * 2 * Pi / kMaxCorners);
      g_normalsOrg[row][corner].z = sin(corner * 2 * Pi / kMaxCorners);
    };

  // g_poly sets the indicies for the triangles
  for (row = 0; row < kMaxRow - 1; row++)
    for (corner = 0; corner < kMaxCorners; corner++) {
      // Quads built from two triangles

      if (corner < kMaxCorners - 1) {
        cornerIndex = row * kMaxCorners + corner;
        g_poly[cornerIndex * 2].v1 = cornerIndex;
        g_poly[cornerIndex * 2].v2 = cornerIndex + 1;
        g_poly[cornerIndex * 2].v3 = cornerIndex + kMaxCorners + 1;

        g_poly[cornerIndex * 2 + 1].v1 = cornerIndex;
        g_poly[cornerIndex * 2 + 1].v2 = cornerIndex + kMaxCorners + 1;
        g_poly[cornerIndex * 2 + 1].v3 = cornerIndex + kMaxCorners;
      } else {  // Special case: Last in the turn, go over the edge properly
        cornerIndex = row * kMaxCorners + corner;
        g_poly[cornerIndex * 2].v1 = cornerIndex;
        g_poly[cornerIndex * 2].v2 = cornerIndex + 1 - kMaxCorners;
        g_poly[cornerIndex * 2].v3 = cornerIndex + kMaxCorners + 1 - kMaxCorners;

        g_poly[cornerIndex * 2 + 1].v1 = cornerIndex;
        g_poly[cornerIndex * 2 + 1].v2 = cornerIndex + kMaxCorners + 1 - kMaxCorners;
        g_poly[cornerIndex * 2 + 1].v3 = cornerIndex + kMaxCorners;
      }
    }

  // Put a copy of the original in g_vertsRes
  memcpy(g_vertsRes, g_vertsOrg, kMaxRow * kMaxCorners * sizeof(vec3));
  memcpy(g_normalsRes, g_normalsOrg, kMaxRow * kMaxCorners * sizeof(vec3));
}

//////////////////////////////////////
//		B O N E
// Desc:  A simple bone structure with position and rotation.
//        rot could have been mat3 but matrix creation in VectorUtils
//        only supports mat4. (We could also cast from that, of course.)
typedef struct Bone {
  vec3 pos;
  mat4 rot;
} Bone;

///////////////////////////////////////
//		G _ B O N E S
// Our "skeleton"
Bone g_bones[kMaxBones];     // Original data, do not change
Bone g_bonesRes[kMaxBones];  // For animation, change to animate

///////////////////////////////////////////////////////
//		S E T U P  B O N E S
//
void setupBones(void) {
  int bone;

  for (bone = 0; bone < kMaxBones; bone++) {
    g_bones[bone].pos = vec3((float)bone * BONE_LENGTH, 0.0f, 0.0f);
    g_bones[bone].rot = IdentityMatrix();
  }
}

void generateMi() {
  mat4 M_bone[kMaxBones], M_bone_prime[kMaxBones];

  // create relative translation matrices for all bones
  T_bones[0] = T(g_bones[0].pos.x, g_bones[0].pos.y, g_bones[0].pos.z);
  for (int i = 1; i < kMaxBones; i++) {
    float delta_x = g_bones[i].pos.x - g_bones[i - 1].pos.x;
    float delta_y = g_bones[i].pos.y - g_bones[i - 1].pos.y;
    float delta_z = g_bones[i].pos.z - g_bones[i - 1].pos.z;
    T_bones[i] = T(delta_x, delta_y, delta_z);
  }

  // create M_bone and M_bone_prime matrices for all bones
  for (int i = 0; i < kMaxBones; i++) {
    M_bone[i] = Mult(T_bones[i], g_bones[i].rot);
    M_bone_prime[i] = Mult(M_bone[i], g_bonesRes[i].rot);
  }

  for (int i = 0; i < kMaxBones; i++) {
    M_i[i] = IdentityMatrix();  // Initialize with identity matrix
    // Forward multiplication
    for (int j = 0; j <= i; j++) {
      M_i[i] = M_i[i] * M_bone_prime[j];
    }
    // Backward multiplication of inverse matrices
    for (int j = i; j >= 0; j--) {
      M_i[i] = M_i[i] * InvertMat4(M_bone[j]);
    }
  }
}

///////////////////////////////////////////////////////
//		D E F O R M  C Y L I N D E R
//
// Desc:	deform the cylinder mesh according to the skeleton
void DeformCylinder() {
  int row, corner;
  // Add more variables as needed

  // for all vertices
  for (row = 0; row < kMaxRow; row++) {
    for (corner = 0; corner < kMaxCorners; corner++) {
      g_vertsRes[row][corner] = g_vertsOrg[row][corner];
      // ---------=========  PART 4 ===========---------
      // TODO: Deform the mesh using all bones
      //
      // data you may use:
      // g_bonesRes[].rot
      // g_bones[].pos
      // g_boneWeights
      // g_vertsOrg
      // g_vertsRes

      vec3 sum = SetVector(0, 0, 0);
      for (int bone = 0; bone < kMaxBones; bone++) {
        sum += g_boneWeights[row][corner][bone] * MultVec3(M_i[bone], g_vertsOrg[row][corner]);
      }
      g_vertsRes[row][corner] = sum;
    }
  }
}

/////////////////////////////////////////////
//		A N I M A T E  B O N E S
// Desc: A simple animation of the skeleton.
//       changes the "rot" by a sin function by the time
void animateBones(void) {
  int bone;
  // How much for each joint? Feel free to edit.
  float angleScales[10] = {1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f};

  float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
  // How much to rotate?
  float angle = sin(time * 3.f) / 2.0f;

  memcpy(&g_bonesRes, &g_bones, kMaxBones * sizeof(Bone));

  g_bonesRes[0].rot = Rz(angle * angleScales[0]);

  for (bone = 1; bone < kMaxBones; bone++) g_bonesRes[bone].rot = Rz(angle * angleScales[bone]);
}

///////////////////////////////////////////////
//		S E T  B O N E  R O T A T I O N
// Desc: sets the bone rotation in the vertex shader.
// (Not mandatory.)
void setBoneRotation(void) {}

///////////////////////////////////////////////
//		 S E T  B O N E  L O C A T I O N
// Desc: sets the bone translation in the vertex shader.
// (Not mandatory.)
void setBoneLocation(void) {}

///////////////////////////////////////////////
//		 D R A W  C Y L I N D E R
// Desc: Upload and draw
void DrawCylinder() {
  animateBones();

  generateMi();

  DeformCylinder();

  setBoneLocation();
  setBoneRotation();

  // update cylinder vertices:
  glBindVertexArray(cylinderModel->vao);
  glBindBuffer(GL_ARRAY_BUFFER, cylinderModel->vb);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * kMaxRow * kMaxCorners, g_vertsRes, GL_DYNAMIC_DRAW);

  DrawModel(cylinderModel, g_shader, "in_Position", "in_Normal", "in_TexCoord");
}

void DisplayWindow() {
  mat4 m;

  glClearColor(0.5, 0.5, 0.9, 1);
  glClear(GL_COLOR_BUFFER_BIT + GL_DEPTH_BUFFER_BIT);

  m = projectionMatrix * modelViewMatrix;
  glUniformMatrix4fv(glGetUniformLocation(g_shader, "matrix"), 1, GL_TRUE, m.m);

  DrawCylinder();

  glutSwapBuffers();
};

void keyboardFunc(unsigned char key, int x, int y) {
  if (key == 27)  // Esc
    exit(1);
}

void reshape(GLsizei w, GLsizei h) {
  vec3 cam = vec3(16, 0, 30);
  vec3 look = vec3(16, 0, 0);

  glViewport(0, 0, w, h);
  GLfloat ratio = (GLfloat)w / (GLfloat)h;
  projectionMatrix = perspective(90, ratio, 0.1, 1000);
  modelViewMatrix = lookAt(cam.x, cam.y, cam.z, look.x, look.y, look.z, 0, 1, 0);
}

/////////////////////////////////////////
//		M A I N
//
int main(int argc, char **argv) {
  glutInit(&argc, argv);

  glutInitWindowSize(800, 800);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitContextVersion(3, 2);
  glutCreateWindow("Them bones, them bones");

  glutDisplayFunc(DisplayWindow);
  glutRepeatingTimer(50);
  glutKeyboardFunc(keyboardFunc);
  glutReshapeFunc(reshape);

  // Set up depth buffer
  glEnable(GL_DEPTH_TEST);

  // initiering
  BuildCylinder();
  setupBones();
  initBoneWeights();

  // Build Model from cylinder data
  cylinderModel = LoadDataToModel((vec3 *)g_vertsRes, (vec3 *)g_normalsRes,
                                  (vec2 *)g_boneWeightVis,  // texCoords
                                  NULL,                     // (GLfloat*) g_boneWeights, // colors
                                  (GLuint *)g_poly,         // indices
                                  kMaxRow * kMaxCorners, kMaxg_poly * 3);

  g_shader = loadShaders("shader.vert", "shader.frag");

  glutMainLoop();
  exit(0);
}
