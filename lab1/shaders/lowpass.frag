#version 150

in vec2 outTexCoord;
uniform sampler2D texUnit;
out vec4 out_Color;

void main(void)
{
    vec2 tex_offset = 1.0 / textureSize(texUnit, 0); // gets size of single texel
    vec3 result = texture(texUnit, outTexCoord).rgb * weight[0]; // current fragment's contribution
    if(horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(texUnit, outTexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(texUnit, outTexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(texUnit, outTexCoord + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(texUnit, outTexCoord - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    out_Color = vec4(result, 1.0);
}