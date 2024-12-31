// ================================================
#shader vertex
// ================================================

#version 330 core

layout(location = 0) in vec3  aInstancePosition;
layout(location = 1) in vec2  aInstanceSize;
layout(location = 2) in float aInstaceType;
layout(location = 3) in float aInstanceDirection;
layout(location = 4) in float aTextureIndex;
layout(location = 5) in vec4 aOcclusion;

layout(location = 6) in vec3 aPos;
layout(location = 7) in vec2 aTexCoords;

uniform mat4 player_camera_projection_matrix;
uniform mat4 player_camera_view_matrix;
uniform mat4 player_camera_model_matrix;
uniform mat4 sun_depth_camera_lightspace_matrix;

out vec3 Normal;
out vec2 TexCoords;
out float TexIndex;
out float Occlusion;

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
    float width = aInstanceSize.x;
    float height = aInstanceSize.y;

    vec3 Size[4] = vec3[4](
        vec3(1    ,height,width ),
        vec3(width,1     ,height),
        vec3(width,height,1     ),
        vec3(1,1,1)
    );

    vec3 pos = aPos * Size[int(aInstaceType)] + aInstancePosition;

    vec4 viewPos = player_camera_view_matrix * player_camera_model_matrix * vec4(pos, 1.0);
    FragPos = viewPos.xyz;
    gl_Position = player_camera_projection_matrix * viewPos;

    int index = int(0);

    Normal = Normals[index];

    vec2 TextCoordList[4] = vec2[4](
        aInstanceSize,
        vec2(height, width),
        aInstanceSize,
        aInstanceSize
    );

    TexCoords = aTexCoords * TextCoordList[int(aInstaceType)];
    TexIndex = aTextureIndex;

    Occlusion = aOcclusion[gl_VertexID % 4];
}

// ================================================
#shader fragment
// ================================================

#version 330 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

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
uniform vec3 sun_direction;
//uniform sampler3D lightArray;

void main()
{
    vec4 full_color = texture(textureArray, vec3(TexCoords, TexIndex));
    if(full_color.a < 0.1) discard;
    
    full_color.rgb = full_color.rgb - (Occlusion / 3);

    gPosition = FragPos;
    gNormal = normalize(Normal);
    gAlbedoSpec.rgb = full_color.rgb;
    gAlbedoSpec.a = full_color.r;
}