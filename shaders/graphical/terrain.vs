#version 330 core

layout(location = 0) in vec3  aPos;
layout(location = 1) in float aNormal;
layout(location = 2) in vec2  aTexCoords;
layout(location = 3) in float aTexIndex;
layout(location = 4) in float aOcclusion;

uniform mat4 player_camera_projection_matrix;
uniform mat4 player_camera_view_matrix;
uniform mat4 player_camera_model_matrix;
uniform mat4 sun_depth_camera_lightspace_matrix;

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
    FragPos = vec3(player_camera_model_matrix * vec4(aPos, 1.0));
    gl_Position = player_camera_projection_matrix * player_camera_view_matrix * vec4(FragPos,1.0);

    int index = int(aNormal);

    Normal = /*transpose(inverse(mat3(player_camera_model_matrix))) */ Normals[index];
    TexCoords = aTexCoords;
    TexIndex = aTexIndex;
    occlusion = aOcclusion;


    FragPosLightSpace = sun_depth_camera_lightspace_matrix * vec4(FragPos - Normal * 0.01, 1.0);
    //gl_Position = vec4(aPos, 1.0);
    
    /*gl_Position = FragPosLightSpace;

    float dist = length(gl_Position.xy);  // Distance from the center
    gl_Position.xy /= (dist+.1);*/


    //gl_Position = FragPosLightSpace;
    //float dist = length(gl_Position.xy);
    //gl_Position.xy /= (dist+.1);
}