#version 330 core
layout(location = 0) in vec3 aInstancePosition;
layout(location = 1) in vec3 aInstanceScale;
layout(location = 2) in vec4 aInstanceRotation;
layout(location = 3) in vec3 aInstanceRotationOffset;

layout(location = 4) in vec3 aLastInstancePosition;
layout(location = 5) in vec3 aLastInstanceScale;
layout(location = 6) in vec4 aLastInstanceRotation;
layout(location = 7) in vec3 aLastInstanceRotationOffset;

layout(location = 8) in vec3 aPos;
layout(location = 9) in vec3 aNormal;
layout(location = 10) in vec2 aTexCoords;
layout(location = 11) in float aIsSolidColor;
layout(location = 12) in vec3 aSolidColor;
layout(location = 13) in float aPixelPerfectSampling;

uniform mat4 player_camera_projection_matrix;
uniform mat4 player_camera_view_matrix;
uniform float model_interpolation_time;

out vec3 FragPos;
out vec2 TexCoords;
out float IsSolidColor;
out vec3 SolidColor;
out vec3 Normal;
out float PixelPerfectSampling;

// Chato
vec3 rotate(vec3 v, vec4 q) {
    vec3 temp = cross(q.xyz, v) + q.w * v;
    return v + 2.0 * cross(q.xyz, temp);
}

void main()
{
    vec3 position = mix(aLastInstancePosition, aInstancePosition, model_interpolation_time);
    vec3 scale = mix(aLastInstanceScale, aInstanceScale, model_interpolation_time);
    vec4 rotation = mix(aLastInstanceRotation, aInstanceRotation, model_interpolation_time);
    vec3 rotation_offset = mix(aLastInstanceRotationOffset, aInstanceRotationOffset, model_interpolation_time);

    vec3 final_position = rotate(aPos + rotation_offset, rotation) * scale + position;
    Normal = rotate(aNormal + rotation_offset, rotation) * scale;

    vec4 viewPos = player_camera_view_matrix * vec4(final_position, 1.0);
    FragPos = viewPos.xyz;
    gl_Position = player_camera_projection_matrix * viewPos;

    gl_Position = aInstanceScale.x == 0 && aInstanceScale.y == 0 && aInstanceScale.z == 0 ? vec4(2.0, 0.0, 0.0, 1.0) : gl_Position;

    TexCoords = aTexCoords;
    IsSolidColor = aIsSolidColor;
    SolidColor = aSolidColor;
    PixelPerfectSampling = aPixelPerfectSampling;
}
