#version 330 core

in vec2 TexCoords;
in vec3 Color;

out vec4 FragColor;

const float borderWidth = 0.01;


void main()
{   
    bool border = TexCoords.x < borderWidth || TexCoords.y < borderWidth || TexCoords.x > 1 - borderWidth || TexCoords.y > 1 - borderWidth;
    
    
    float a = border ? 0 : 1;
    FragColor = vec4(Color * a, 1.0);  // Output a solid red color
}