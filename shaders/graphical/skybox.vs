#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;
uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform vec3 sunPos;

out vec4 something;
void main()
{
    TexCoords = aPos;

    mat4 view = mat4(mat3(viewMatrix));  // Remove translation component
    vec4 pos = projectionMatrix * view * vec4(aPos, 1.0);
    
    gl_Position = pos.xyww;
    //gl_Position = pos;
}