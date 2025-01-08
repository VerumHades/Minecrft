#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;
uniform mat4 player_camera_projection_matrix;
uniform mat4 player_camera_view_matrix;
uniform vec3 sunPos;

out vec4 something;
void main()
{
    TexCoords = aPos;

    mat4 view = mat4(mat3(player_camera_view_matrix));
    vec4 pos = player_camera_projection_matrix * view * vec4(aPos, 1.0);
    
    gl_Position = pos.xyww;
}