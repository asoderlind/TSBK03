// 2022: C++ version.

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#define MAIN
#include "MicroGlut.h"
#include "VectorUtils4.h"
#include "GL_utilities.h"
#include "LittleOBJLoader.h"
// uses framework Cocoa
// uses framework OpenGL

// Ref till shader
GLuint g_shader;

typedef struct Triangle
{
	GLuint				v1;
	GLuint				v2;
	GLuint				v3;
} Triangle;

#define kMaxRow 10
#define kMaxCorners 8
#define kMaxg_poly ((kMaxRow-1) * kMaxCorners * 2)
#ifndef Pi
#define Pi 3.1416
#endif
#ifndef true
#define true 1
#endif


Triangle g_poly[kMaxg_poly];

// vertices
vec3 g_vertsOrg[kMaxRow][kMaxCorners];
vec3 g_normalsOrg[kMaxRow][kMaxCorners];

// vertices sent to OpenGL
vec3 g_vertsRes[kMaxRow][kMaxCorners];
vec3 g_normalsRes[kMaxRow][kMaxCorners];

// vertex attributes sent to OpenGL
vec3 g_boneWeights[kMaxRow][kMaxCorners];

float weight[kMaxRow] = {0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0};

Model *cylinderModel; // Collects all the above for drawing with glDrawElements

mat4 modelViewMatrix, projectionMatrix;

///////////////////////////////////////////////////
//		B U I L D	C Y L I N D E R
// Desc: Builds the geometry
//
void BuildCylinder()
{
	long	row, corner, cornerIndex;
	float g_vertstex[kMaxRow][kMaxCorners][2];

	// Sets the values for all vertices in the mesh
	for (row = 0; row < kMaxRow; row++)
		for (corner = 0; corner < kMaxCorners; corner++)
		{
			g_vertsOrg[row][corner].x = row;
			g_vertsOrg[row][corner].y = cos(corner * 2*Pi / kMaxCorners);
			g_vertsOrg[row][corner].z = sin(corner * 2*Pi / kMaxCorners);

			g_normalsOrg[row][corner].x = 0;
			g_normalsOrg[row][corner].y = cos(corner * 2*Pi / kMaxCorners);
			g_normalsOrg[row][corner].z = sin(corner * 2*Pi / kMaxCorners);

			g_boneWeights[row][corner].x = (1-weight[row]);
			g_boneWeights[row][corner].y = weight[row];
			g_boneWeights[row][corner].z = 0.0;
		};

  // g_poly sets the indicies for the triangles
	for (row = 0; row < kMaxRow-1; row++)
		for (corner = 0; corner < kMaxCorners; corner++)
		{
	// Quads built from two triangles

			if (corner < kMaxCorners-1) 
			{
				cornerIndex = row * kMaxCorners + corner;
				g_poly[cornerIndex * 2].v1 = cornerIndex;
				g_poly[cornerIndex * 2].v2 = cornerIndex + 1;
				g_poly[cornerIndex * 2].v3 = cornerIndex + kMaxCorners + 1;
	
				g_poly[cornerIndex * 2 + 1].v1 = cornerIndex;
				g_poly[cornerIndex * 2 + 1].v2 = cornerIndex + kMaxCorners + 1;
				g_poly[cornerIndex * 2 + 1].v3 = cornerIndex + kMaxCorners;
			}
			else
	  { // Special case: Last in the turn, go over the edge properly
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

	for (row = 0; row < kMaxRow; row++)
		for (corner = 0; corner < kMaxCorners; corner++)
		{
			g_vertsRes[row][corner] = g_vertsOrg[row][corner];
			g_normalsRes[row][corner] = g_normalsOrg[row][corner];
			g_vertstex[row][corner][0]=(1-weight[row]);
			g_vertstex[row][corner][1]=weight[row];
		}
	
	// Build Model from cylinder data
	cylinderModel = LoadDataToModel(
			(vec3*) g_vertsRes,
			(vec3*) g_normalsRes,
			(vec2*) g_vertstex, // texCoords
			(vec3*) g_vertstex, // colors
			(GLuint*) g_poly, // indices
			kMaxRow*kMaxCorners,
			kMaxg_poly * 3);
}


//////////////////////////////////////
//		B O N E
// Desc:  A simple bone structure with position and rotation.
//        rot could have been mat3 but matrix creation in VectorUtils
//        only supports mat4. (We could also cast from that, of course.)
typedef struct Bone
{
	vec3 pos;
	mat4 rot;
} Bone;

///////////////////////////////////////
//		G _ B O N E S
// Our "skeleton", just two bones
Bone g_bones[2];


///////////////////////////////////////////////////////
//		S E T U P	B O N E S
//
// Desc:	Sets bone 0 at origin and
//			bone 1 at pos (4.5, 0, 0)
void setupBones(void)
{
	g_bones[0].pos = SetVector(0.0f, 0.0f, 0.0f);
	g_bones[1].pos = SetVector(4.5f, 0.0f, 0.0f);
	g_bones[0].rot = IdentityMatrix();
	g_bones[1].rot = IdentityMatrix();
}


///////////////////////////////////////////////////////
//		D E F O R M	C Y L I N D E R 
//
// Desc:	deform the cylinder mesh according to the skeleton
void DeformCylinder()
{
	// vec3 v1, v2;
	int row, corner;
	
	// för samtliga vertexar 
	for (row = 0; row < kMaxRow; row++)
	{
		for (corner = 0; corner < kMaxCorners; corner++)
		{
			g_vertsRes[row][corner] = g_vertsOrg[row][corner];
			
			// ----=========	Part 1: Stitching in CPU ===========-----
			// Deform the cylindern from the skeleton in g_bones.
			//
			// Perform stitching.
			//
			// g_bones holds the bones.
			// g_vertsOrg are original vertex data.
			// g_vertsRes are modified vertex data to send to OpenGL.
			//
			// row goes long the length of the cylinder,
			// corner around each layer.
			
			
			// ---=========	Part 2: Skinning in CPU ===========------
			// Deform the cylindern from the skeleton in g_bones.
			// i g_bones.
			//
			// Perform skinning.
			//
			// g_bones holds the bones.
			// g_boneWeights are blending weights for the bones.
			// g_vertsOrg are original vertex data.
			// g_vertsRes are modified vertex data to send to OpenGL.
			
		}
	}
}


/////////////////////////////////////////////
//		A N I M A T E  B O N E S
// Desc: A simple animation of the skeleton.
//       changes the "rot" by a sin function by the time
void animateBones(void)
{
	float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0;

	// How much to rotate?
	float angle = sin(time * 3.f) / 2.0f * 3.0f;

	// rotare bone 1
	g_bones[1].rot = Rz(angle);
}


///////////////////////////////////////////////
//		S E T	B O N E	R O T A T I O N
// Desc: sets the bone rotation in the vertex shader.
void setBoneRotation(void)
{
	// Uppgift 3 TODO: Here you can send the bone rotation
	// to the vertex shader
}


///////////////////////////////////////////////
//		 S E T	B O N E	L O C A T I O N
// Desc: sets the bone position in the vertex shader.
void setBoneLocation(void)
{
	// Uppgift 3 TODO: Here you can send the bone position
	// to the vertex shader
}


///////////////////////////////////////////////
//		 D R A W	C Y L I N D E R
// Desc: Upload and draw
void DrawCylinder()
{
	animateBones();

	// ---------=========	UPG 2 ===========---------
	// Move the vertex calculations from DeformCylinder into a vertex shader.
	// The current one is "shader.vert".
	
	DeformCylinder();
	
	setBoneLocation();
	setBoneRotation();
	
// update cylinder vertices:
	glBindVertexArray(cylinderModel->vao);
	glBindBuffer(GL_ARRAY_BUFFER, cylinderModel->vb);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*kMaxRow*kMaxCorners, g_vertsRes, GL_DYNAMIC_DRAW);
	
	DrawModel(cylinderModel, g_shader, "in_Position", "in_Normal", "in_TexCoord");
}


void DisplayWindow()
{
	mat4 m;
	
	glClearColor(0.5, 0.5, 0.9, 1);
	glClear(GL_COLOR_BUFFER_BIT+GL_DEPTH_BUFFER_BIT);

    m = projectionMatrix * modelViewMatrix;
    glUniformMatrix4fv(glGetUniformLocation(g_shader, "matrix"), 1, GL_TRUE, m.m);
	
	DrawCylinder();

	glutSwapBuffers();
}

void keyboardFunc( unsigned char key, int x, int y)
{
// Add any keyboard control you want here
	if(key == 27)	//Esc
		exit(-1);
}

void reshape(GLsizei w, GLsizei h)
{
	vec3 cam = vec3(5,0,8);
	vec3 look = vec3(5,0,0);

    glViewport(0, 0, w, h);
    GLfloat ratio = (GLfloat) w / (GLfloat) h;
    projectionMatrix = perspective(90, ratio, 0.1, 1000);
	modelViewMatrix = lookAt(cam.x, cam.y, cam.z,
							look.x, look.y, look.z, 
							0,1,0);
}

/////////////////////////////////////////
//		M A I N
//
int main(int argc, char **argv)
{
	glutInit(&argc, argv);

	glutInitWindowSize(800, 800);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitContextVersion(3, 2);
	glutCreateWindow("Them bones");

	glutDisplayFunc(DisplayWindow);
	glutRepeatingTimer(50);
	glutKeyboardFunc( keyboardFunc ); 
	glutReshapeFunc(reshape);

	// Set up depth buffer
	glEnable(GL_DEPTH_TEST);

	// initiering
	BuildCylinder();
	setupBones();
	g_shader = loadShaders("shader.vert" , "shader.frag");

	glutMainLoop();
	exit(0);
}
