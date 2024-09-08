
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 lightSpaceMatrix;
uniform mat4 modelMatrix;

uniform mat4 cuboidMatrices[64];

void main()
{
    mat4 current = cuboidMatrices[gl_InstanceID];
    gl_Position = lightSpaceMatrix * modelMatrix * current * vec4(aPos, 1.0);

    //float dist = distance(gl_Position.xz, camPos.xz);
    //gl_Position.xy /= dist;

    // Warp the shadow map to increase resolution close to the player
    float dist = length(gl_Position.xy);
    gl_Position.xy /= (dist+.1);
}  