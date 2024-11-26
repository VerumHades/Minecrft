#version 330 core
layout (location = 0) in vec3  aPos; 
layout (location = 1) in vec2  aTexCoords; 
layout (location = 2) in vec4  aColor; 
layout (location = 3) in vec2  aSize; 
layout (location = 4) in float aIsText; 
layout (location = 5) in float aIsTexture; 
layout (location = 6) in float aTextureIndex; 
layout (location = 7) in vec4  aBorderWidth; 
layout (location = 8) in vec4  aClipRegion; 

layout (location = 9)  in vec4  aBorderColorTop; 
layout (location = 10)  in vec4  aBorderColorRight; 
layout (location = 11) in vec4  aBorderColorBottom; 
layout (location = 12) in vec4  aBorderColorLeft; 

out vec2 TexCoords;
out vec2 Size;
out float isText;
out vec4 Color;
out float isTexture;
out float textureIndex;
out vec4 borderWidth;

out vec4 clipRegion;
out vec3 screenPos;

out vec4 borderColorTop;
out vec4 borderColorRight;
out vec4 borderColorBottom;
out vec4 borderColorLeft;

uniform mat4 ui_projection_matrix;

void main()
{
    gl_Position = ui_projection_matrix * vec4(aPos.xy, 0.0, 1.0);
    //gl_Position.z = aPos.z;
    
    screenPos = aPos;

    TexCoords = aTexCoords;
    Color = aColor;
    isText = aIsText;
    Size = aSize;
    isTexture = aIsTexture;
    textureIndex = aTextureIndex;
    borderWidth = aBorderWidth;

    clipRegion = aClipRegion;

    borderColorTop = aBorderColorTop;
    borderColorRight = aBorderColorRight;
    borderColorBottom = aBorderColorBottom;
    borderColorLeft = aBorderColorLeft;
}