#version 330 core
layout (location = 0) in vec3 aPos;  
layout (location = 1) in vec3 aOffset;
layout (location = 2) in vec3 aScale;
layout (location = 3) in vec3 aColor; 

out vec3 instanceColor; 
out vec3 FragPos;

uniform mat4 player_camera_projection_matrix;
uniform mat4 player_camera_view_matrix;

void main() {
    instanceColor = aColor;  

    vec4 viewPos = player_camera_view_matrix  * vec4(aPos * aScale + aOffset, 1.0);
    FragPos = viewPos.xyz;
    gl_Position = player_camera_projection_matrix * viewPos;
}
