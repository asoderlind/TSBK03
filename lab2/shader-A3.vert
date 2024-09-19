#version 150

//in vec3 in_Color;
in vec3 in_Position;
in vec3 in_Normal;
in vec2 in_TexCoord;
uniform mat4 M_bone0;
uniform mat4 M_bone1;
uniform mat4 matrix;

out vec4 g_color;
const vec3 lightDir = normalize(vec3(0.3, 0.5, 1.0));

// Uppgift 3: Soft-skinning p� GPU
//
// Flytta �ver din implementation av soft skinning fr�n CPU-sidan
// till vertexshadern. Mer info finns p� hemsidan.

void main(void)
{
	// transformera resultatet med ModelView- och Projection-matriserna
	vec4 res_Position = in_TexCoord[0] * M_bone0 * vec4(in_Position, 1) + in_TexCoord[1] * M_bone1 * vec4(in_Position,1);
	gl_Position = matrix * res_Position;

	// s�tt r�d+gr�n f�rgkanal till vertex Weights
	vec4 color = vec4(in_TexCoord.x, in_TexCoord.y, 0.0, 1.0);

	// L�gg p� en enkel ljuss�ttning p� vertexarna 	
	float intensity = dot(in_Normal, lightDir);
	color.xyz *= intensity;

	g_color = color;
}

