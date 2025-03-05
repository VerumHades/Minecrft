#version 330 core

layout (location = 0) in vec3 aInstancePosition;
layout (location = 1) in float aInstanceTextureIndex;

layout (location = 2) in vec3 aPos;  
layout (location = 3) in vec3 aNormal;
layout (location = 4) in vec2 aTexCoord;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;

uniform mat4 player_camera_projection_matrix;
uniform mat4 player_camera_view_matrix;

uniform float cube_renderer_texture_count;

void main() {
    float textureOffset = aInstanceTextureIndex / cube_renderer_texture_count;
    float textureWidth = 1.0 / cube_renderer_texture_count;

    Normal = aNormal;
    TexCoords = vec2(aTexCoord.x * textureWidth + textureOffset, aTexCoord.y);

    vec4 viewPos = player_camera_view_matrix  * vec4(aPos * 1.02 + (aInstancePosition - 0.01), 1.0);
    FragPos = viewPos.xyz;
    gl_Position = player_camera_projection_matrix * viewPos;
}
