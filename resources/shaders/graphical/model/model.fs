#version 330 core

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec4 gAlbedoSpec;

precision highp float;

in vec3 FragPos;
in vec2 TexCoords;
in float IsSolidColor;
in vec3 SolidColor;
in vec3 Normal;
in float PixelPerfectSampling;

uniform sampler2D textureIn;

void main()
{
    float sprite_size = PixelPerfectSampling;

    vec2 pixel_position = vec2(floor(TexCoords * sprite_size)) + 0.5;
    vec2 sample_position = vec2(pixel_position) / sprite_size;

    vec4 full_color = IsSolidColor > 0.5 ? vec4(SolidColor, 1.0) : texture(textureIn, PixelPerfectSampling > 0.5 ? sample_position : TexCoords);
    if (full_color.a == 0) discard;

    gPosition = FragPos;
    gNormal = normalize(Normal);
    gAlbedoSpec.rgb = full_color.rgb;
    gAlbedoSpec.a = full_color.a;
}
