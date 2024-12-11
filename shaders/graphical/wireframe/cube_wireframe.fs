#version 330 core
in vec3 instanceColor;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;

void main() {
    gPosition = FragPos;
    gNormal = normalize(vec3(0,1,0));
    gAlbedoSpec.rgb = instanceColor.rgb;
    gAlbedoSpec.a = instanceColor.r;
}