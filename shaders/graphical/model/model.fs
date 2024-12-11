#version 330 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

precision highp float; 

in vec3 FragPos;
in vec2 TexCoords;
in float IsSolidColor;
in vec3 SolidColor;
in vec3 Normal;

uniform sampler2D textureIn;

void main()
{
    //vec2 pixel_position = floor(TexCoords * ) + 0.5;

    const float sprite_size = 32.0;
    
    vec2 pixel_position  = vec2(floor(TexCoords * sprite_size)) + 0.5;
    vec2 sample_position = vec2(pixel_position) / sprite_size;

    //FragColor = vec4(pixel_position,0.5,1.0);
    vec4 full_color = IsSolidColor > 0.5 ? vec4(SolidColor, 1.0) : texture(textureIn, sample_position);
    if(full_color.a == 0) discard;

    gPosition = FragPos;
    gNormal = normalize(Normal);
    gAlbedoSpec.rgb = full_color.rgb;
    gAlbedoSpec.a = full_color.r;
}