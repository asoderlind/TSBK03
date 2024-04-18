#version 150

in vec2 outTexCoord;
uniform sampler2D texUnit;
uniform sampler2D glow;
out vec4 out_Color;

void main(void)
{
    out_Color = 0.7*texture(texUnit, outTexCoord) + 1.0*texture(glow, outTexCoord);
}