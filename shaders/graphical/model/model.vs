#version 330 core
layout(location = 0) in vec3 aInstanceOffset;

layout(location = 1) in vec3 aPos;

uniform mat4 player_camera_projection_matrix;
uniform mat4 player_camera_view_matrix;
uniform mat4 player_camera_model_matrix;

out vec3 FragPos;

void main()
{
    FragPos = vec3(player_camera_model_matrix * vec4(aPos + aInstanceOffset, 1.0));
    gl_Position = player_camera_projection_matrix * player_camera_view_matrix * vec4(FragPos,1.0);

    /*FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    //gl_Position = vec4(aPos, 1.0);
    
    gl_Position = FragPosLightSpace;

    float dist = length(gl_Position.xy);  // Distance from the center
    gl_Position.xy /= (dist+.1);*/
}