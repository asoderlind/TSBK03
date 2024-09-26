#version 150

in vec3 in_Position;
in vec3 in_Normal;

uniform mat4 viewMatrix, modelToWorldMatrix;
uniform mat4 projMatrix;

out vec2 outTexCoord;
out vec3 pixPos;
out vec3 out_Normal;

void main(void)
{
    outTexCoord.x = in_Position.x*5.5+.5; // Hard coded adjustment for small model
    outTexCoord.y = in_Position.z*5.5+.5;
    pixPos = vec3(viewMatrix * modelToWorldMatrix * vec4(in_Position, 1.0));
    out_Normal = mat3(viewMatrix * modelToWorldMatrix) * in_Normal;

	gl_Position = projMatrix * viewMatrix * modelToWorldMatrix * vec4(in_Position, 1.0);
}
