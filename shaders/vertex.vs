#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormals;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in float aTexIndex;
layout(location = 4) in vec3 aLightLevel;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 Normals;
out vec2 TexCoords;
out float TexIndex;
out vec3 LightLevel;

out vec3 crntPosition;

void main()
{
    crntPosition = vec3(model * vec4(aPos, 1.0));

    gl_Position = projection * view * vec4(crntPosition,1.0);
    Normals = aNormals;
    TexCoords = aTexCoords;
    TexIndex = aTexIndex;
    LightLevel = aLightLevel;
    //gl_Position = vec4(aPos, 1.0);
}