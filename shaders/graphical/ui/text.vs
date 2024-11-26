#version 330 core
layout (location = 0) in vec4 vertex; // (position, texCoords)

out vec2 TexCoords;

uniform mat4 ui_projection_matrix;

void main()
{
    gl_Position = ui_projection_matrix * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}