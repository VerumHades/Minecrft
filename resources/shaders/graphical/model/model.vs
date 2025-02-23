#version 330 core
layout(location = 0) in vec4 aInstanceMatrixComponent1;
layout(location = 1) in vec4 aInstanceMatrixComponent2;
layout(location = 2) in vec4 aInstanceMatrixComponent3;
layout(location = 3) in vec4 aInstanceMatrixComponent4;

layout(location = 4) in vec4 aLastInstanceMatrixComponent1;
layout(location = 5) in vec4 aLastInstanceMatrixComponent2;
layout(location = 6) in vec4 aLastInstanceMatrixComponent3;
layout(location = 7) in vec4 aLastInstanceMatrixComponent4;

layout(location = 8) in vec3  aPos;
layout(location = 9) in vec3  aNormal;
layout(location = 10) in vec2  aTexCoords;
layout(location = 11) in float aIsSolidColor;
layout(location = 12) in vec3  aSolidColor;
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

void main()
{
    mat4 modelMatrix = mat4(aInstanceMatrixComponent1,aInstanceMatrixComponent2,aInstanceMatrixComponent3,aInstanceMatrixComponent4);
    mat4 lastModelMatrix = mat4(aLastInstanceMatrixComponent1,aLastInstanceMatrixComponent2,aLastInstanceMatrixComponent3,aLastInstanceMatrixComponent4);

    Normal = transpose(inverse(mat3(modelMatrix))) * aNormal;

    vec4 viewPos = player_camera_view_matrix * modelMatrix * vec4(aPos, 1.0);
    FragPos = viewPos.xyz;
    gl_Position = player_camera_projection_matrix * viewPos;

    TexCoords = aTexCoords;
    IsSolidColor = aIsSolidColor;
    SolidColor = aSolidColor;
    PixelPerfectSampling = aPixelPerfectSampling;
}