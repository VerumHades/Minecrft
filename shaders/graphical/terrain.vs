#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in float aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in float aTexIndex;
layout(location = 4) in float aOcclusion;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform mat4 lightSpaceMatrix;

out vec3 Normal;
out vec2 TexCoords;
out float TexIndex;
out vec4 FragPosLightSpace;
out float occlusion;

out vec3 FragPos;
uniform vec3 camPos;

const vec3 Normals[6] = vec3[6](
    vec3( 0, 1, 0),
    vec3( 0,-1, 0),
    vec3( 1, 0, 0),
    vec3(-1, 0, 0),
    vec3( 0, 0, 1),
    vec3( 0, 0,-1)
);

void main()
{
    FragPos = vec3(modelMatrix * vec4(aPos, 1.0));
    gl_Position = projectionMatrix * viewMatrix * vec4(FragPos,1.0);

    int index = int(aNormal);

    Normal = /*transpose(inverse(mat3(modelMatrix))) */ Normals[index];
    TexCoords = aTexCoords;
    TexIndex = aTexIndex;
    occlusion = aOcclusion;


    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos - Normal * 0.01, 1.0);
    //gl_Position = vec4(aPos, 1.0);
    
    /*gl_Position = FragPosLightSpace;

    float dist = length(gl_Position.xy);  // Distance from the center
    gl_Position.xy /= (dist+.1);*/


    //gl_Position = FragPosLightSpace;
    //float dist = length(gl_Position.xy);
    //gl_Position.xy /= (dist+.1);
}