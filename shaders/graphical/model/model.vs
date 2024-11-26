#version 330 core
layout(location = 0) in vec4 aInstanceMatrixComponent1;
layout(location = 1) in vec4 aInstanceMatrixComponent2;
layout(location = 2) in vec4 aInstanceMatrixComponent3;
layout(location = 3) in vec4 aInstanceMatrixComponent4;

layout(location = 4) in vec3 aPos;

uniform mat4 player_camera_projection_matrix;
uniform mat4 player_camera_view_matrix;

out vec3 FragPos;

void main()
{
    mat4 modelMatrix = mat4(aInstanceMatrixComponent1,aInstanceMatrixComponent2,aInstanceMatrixComponent3,aInstanceMatrixComponent4);

    FragPos = vec3(modelMatrix * vec4(aPos, 1.0));
    gl_Position = player_camera_projection_matrix * player_camera_view_matrix * vec4(FragPos,1.0);

    /*FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    //gl_Position = vec4(aPos, 1.0);
    
    gl_Position = FragPosLightSpace;

    float dist = length(gl_Position.xy);  // Distance from the center
    gl_Position.xy /= (dist+.1);*/
}