#version 330 core

in vec2 TexCoords;
in vec2 Size;
in float isText;
in vec3 Color;

out vec4 FragColor;

const float borderWidth = 0.01;

uniform sampler2DArray tex;
uniform sampler2D textAtlas;

void main()
{   
    bool border = TexCoords.x < borderWidth || TexCoords.y < borderWidth || TexCoords.x > 1 - borderWidth || TexCoords.y > 1 - borderWidth;
    
    float a = border ? 0 : 1;

    vec4 sampled = isText > 0.5 ? vec4(1.0, 1.0, 1.0, texture(textAtlas, TexCoords).r) : vec4(1);

    FragColor = vec4(Color, 1.0) * sampled;  // Output a solid red color
}