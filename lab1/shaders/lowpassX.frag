#version 150

in vec2 outTexCoord;
uniform sampler2D texUnit;
out vec4 out_Color;

void main(void)
{
    int size = 4;
    vec2 tex_offset = 1.0 / textureSize(texUnit, 0); // gets size of single texel
    vec3 result = vec3(0.0);
    
    result += texture(texUnit, outTexCoord - vec2(tex_offset.x, 0.0)).rgb;
    result += 2.0 * texture(texUnit, outTexCoord).rgb;
    result += texture(texUnit, outTexCoord + vec2(tex_offset.x, 0.0)).rgb;

    out_Color = vec4(result/size, 1.0);
}