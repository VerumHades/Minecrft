#version 330 core

precision highp float; 

in vec3 FragPos;
in vec2 TexCoords;
in float IsSolidColor;
in vec3 SolidColor;

out vec4 FragColor;

uniform sampler2D textureIn;


void main()
{
    //vec2 pixel_position = floor(TexCoords * ) + 0.5;

    const float sprite_size = 32.0;
    
    vec2 pixel_position  = vec2(floor(TexCoords * sprite_size)) + 0.5;
    vec2 sample_position = vec2(pixel_position) / sprite_size;

    //FragColor = vec4(pixel_position,0.5,1.0);
    FragColor = IsSolidColor > 0.5 ? vec4(SolidColor, 1.0) : texture(textureIn, sample_position);
    if(FragColor.a == 0) discard;
}