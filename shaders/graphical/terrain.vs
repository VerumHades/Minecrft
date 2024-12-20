#version 330 core

layout(location = 0) in vec3  aInstancePosition;
layout(location = 1) in vec2  aInstanceSize;
layout(location = 2) in float aInstanceDirection;
layout(location = 3) in float aTextureIndex;

layout(location = 4) in vec3 aPos;
layout(location = 5) in vec2 aTexCoords;

uniform mat4 player_camera_projection_matrix;
uniform mat4 player_camera_view_matrix;
uniform mat4 player_camera_model_matrix;
uniform mat4 sun_depth_camera_lightspace_matrix;

out vec3 Normal;
out vec2 TexCoords;
out float TexIndex;

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
    aPos = aPos + aInstancePosition;

    vec4 viewPos = player_camera_view_matrix * player_camera_model_matrix * vec4(aPos, 1.0);
    FragPos = viewPos.xyz;
    gl_Position = player_camera_projection_matrix * viewPos;

    int index = int(0);

    Normal = Normals[index];
    TexCoords = aTexCoords;
    TexIndex = aTextureIndex;
}