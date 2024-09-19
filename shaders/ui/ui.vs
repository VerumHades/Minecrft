#version 330 core
layout (location = 0) in vec2 aPos; 
layout (location = 1) in vec2 aTexCoords; 
layout (location = 2) in vec3 aColor; 

out vec2 TexCoords;
out vec3 Color;

uniform mat4 projectionMatrix;

void main()
{
    gl_Position = projectionMatrix * vec4(aPos, 0.0, 1.0);
    TexCoords = aTexCoords;
    Color = aColor;
}