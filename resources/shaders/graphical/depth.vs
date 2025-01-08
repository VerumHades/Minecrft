
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 sun_depth_camera_lightspace_matrix;
uniform mat4 sun_depth_camera_model_matrix;
uniform vec3 player_camera_position;

void main()
{
    gl_Position = sun_depth_camera_lightspace_matrix * sun_depth_camera_model_matrix * vec4(aPos, 1.0);

    //float dist = distance(gl_Position.xz, camPos.xz);
    //gl_Position.xy /= 0.5;

    // Warp the shadow map to increase resolution close to the player
    //float dist = length(gl_Position.xy);
    //gl_Position.xy /= (dist+.1);
}  