#version 330 core
layout (location = 0) in vec2  aPos; 
layout (location = 1) in vec4  aColor; 
layout (location = 2) in vec2  aTexCoords;
layout (location = 3) in float aType; 

out vec2 TexCoords;
out vec4 Color;
out float Type;

uniform mat4 ui_projection_matrix;

void main()
{
    gl_Position = ui_projection_matrix * vec4(aPos.xy, 0.0, 1.0);
 

    TexCoords = aTexCoords;
    Color = aColor;
    Type = aType;
}