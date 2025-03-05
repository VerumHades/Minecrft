#version 330 core
in vec3 instanceColor;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D cube_texture_atlas;

void main() {
    gPosition = FragPos;
    gNormal = normalize(Normal);

    vec4 color = texture(cube_texture_atlas, TexCoords);
    if(color.a == 0) discard;

    gAlbedoSpec.rgb = color.rgb;
    gAlbedoSpec.a = color.r;
}