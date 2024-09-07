#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform mat4 lightSpaceMatrix;

out vec3 Normal;
out vec2 TexCoords;
out float TexIndex;
out vec4 FragPosLightSpace;

out vec3 FragPos;

uniform mat4 cuboidMatrices[64];
uniform mat3 textureCoordinates[64]; 

void main()
{
    mat4 current = cuboidMatrices[gl_InstanceID];
    int vertIndex = gl_VertexID * 2;
    
    int col = vertIndex % 3;
    int row = vertIndex / 3;

    vertIndex++;

    int col2 = vertIndex % 3;
    int row2 = vertIndex / 3;

    mat3 texCoordMat = textureCoordinates[gl_InstanceID];
    float texX = texCoordMat[col][row];
    float texY = texCoordMat[col2][row2];

    FragPos = vec3(modelMatrix * current * vec4(aPos, 1.0));
    gl_Position = projectionMatrix * viewMatrix * vec4(FragPos,1.0);
    
    Normal = transpose(inverse(mat3(modelMatrix))) * aNormal;
    TexCoords = vec2(texX, texY);
    TexIndex = texCoordMat[2][2];
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    //gl_Position = vec4(aPos, 1.0);

    //gl_Position = FragPosLightSpace;
}