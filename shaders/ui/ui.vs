#version 330 core
layout (location = 0) in vec2  aPos; 
layout (location = 1) in vec2  aTexCoords; 
layout (location = 2) in vec3  aColor; 
layout (location = 3) in vec2  aSize; 
layout (location = 4) in float aIsText; 
layout (location = 5) in float aIsTexture; 
layout (location = 6) in float aTextureIndex; 

out vec2 TexCoords;
out vec2 Size;
out float isText;
out vec3 Color;
out float isTexture;
out float textureIndex;

uniform mat4 projectionMatrix;

void main()
{
    gl_Position = projectionMatrix * vec4(aPos, 0.0, 1.0);
    TexCoords = aTexCoords;
    Color = aColor;
    isText = aIsText;
    Size = aSize;
    isTexture = aIsTexture;
    textureIndex = aTextureIndex;
}