#version 150

in vec2 outTexCoord;
uniform sampler2D texUnit;
out vec4 out_Color;

void main(void) {
	vec4 col = texture(texUnit, outTexCoord);
	out_Color = max(col, 1.0);
}