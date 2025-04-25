
#version 330 core

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec4 gAlbedoSpec;

precision highp float;

in vec3 Normal;
in vec2 TexCoords;
in float TexIndex;

in vec4 FragPosLightSpace;
in vec3 FragPos;
in vec3 pos;
in float Occlusion;

uniform sampler2DArray textureArray;
uniform sampler2D shadowMap;
//uniform sampler3D lightArray;

void main()
{
    vec4 full_color = texture(textureArray, vec3(TexCoords, TexIndex));
    if (full_color.a < 0.1) discard;

    full_color.rgb = full_color.rgb - ((Occlusion / 6) / 2);

    gPosition = FragPos;
    gNormal = normalize(Normal);
    gAlbedoSpec.rgb = full_color.rgb;
    gAlbedoSpec.a = full_color.a;
}
