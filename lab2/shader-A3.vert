#version 150

//in vec3 in_Color;
in vec3 in_Position;
in vec3 in_Normal;
in vec2 in_TexCoord;
uniform mat4 matrix;
uniform mat4 M_0;
uniform mat4 M_1;

out vec4 g_color;
const vec3 lightDir = normalize(vec3(0.3, 0.5, 1.0));

// Uppgift 3: Soft-skinning p� GPU
//
// Flytta över din implementation av soft skinning från CPU-sidan
// till vertexshadern. Mer info finns på hemsidan.

void main(void)
{
  /* g_vertsRes[row][corner] = g_boneWeights[row][corner].x * M_0 * g_vertsOrg[row][corner] +
                        g_boneWeights[row][corner].y * M_1 * g_vertsOrg[row][corner]; */

	// Lägg ihop de två transformationerna
	vec4 new_pos = in_TexCoord[0] * M_0 * vec4(in_Position, 1.0) + in_TexCoord[1] * M_1 * vec4(in_Position, 1.0);


	// transformera resultatet med ModelView- och Projection-matriserna
	gl_Position = matrix * new_pos;

	// s�tt r�d+gr�n f�rgkanal till vertex Weights
	vec4 color = vec4(in_TexCoord.x, in_TexCoord.y, 0.0, 1.0);

	// L�gg p� en enkel ljuss�ttning p� vertexarna 	
	float intensity = dot(in_Normal, lightDir);
	color.xyz *= intensity;

	g_color = color;
}

