#version 460 core

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

layout(std430, binding = 0) buffer SSBO_positions {
    int world_positions[];
};

layout(std430, binding = 1) buffer SSBO_data {
    uint data[];
};

const vec3 Normals[6] = vec3[6](
    vec3( 0, 1, 0),
    vec3( 0,-1, 0),
    vec3( 1, 0, 0),
    vec3(-1, 0, 0),
    vec3( 0, 0, 1),
    vec3( 0, 0,-1)
);

const vec3 Vertices[12] = vec3[12](
    // X aligned face
    vec3(0.0, -1.0, 0.0),
    vec3(0.0, -1.0, 1.0),
    vec3(0.0,  0.0, 1.0),
    vec3(0.0,  0.0, 0.0),
    // Y aligned face
    vec3(0.0, 0.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(1.0, 0.0, 1.0),
    vec3(1.0, 0.0, 0.0),
    // Z aligned face
    vec3(0.0, -1.0, 0.0),
    vec3(1.0, -1.0, 0.0),
    vec3(1.0,  0.0, 0.0),
    vec3(0.0,  0.0, 0.0)
);

const vec2 NormalizedTexCoords[4] = vec2[4](
    vec2(0,1),
    vec2(1,1),
    vec2(1,0),
    vec2(0,0)
);

const uint Indices[6] = uint[6](
    0,1,3,1,2,3
);

uniform float FaceType;
uniform ivec3 world_position;

void main()
{
    /*ivec3 world_position = ivec3(
        world_positions[gl_DrawID * 3],
        world_positions[gl_DrawID * 3 + 1],
        world_positions[gl_DrawID * 3 + 2]
    );*/
    
    uint id = gl_VertexID;
    uint index = (id / 6) * 2;

    uint first_portion = data[index];
    uint second_portion = data[index + 1];

    const uint mask6 = 63;
    const uint mask7 = 127;
    const uint mask2 = 3;

    uint x      = first_portion & mask6;
    uint y      = (first_portion >> 6 ) & mask7;
    uint z      = (first_portion >> 13) & mask6;

    uint width  = (first_portion >> 19) & mask6;
    uint height = (first_portion >> 25) & mask6;

    width = width > 0 ? width : 64;
    height = height > 0 ? height : 64;

    bool is_forward = bool((first_portion >> 31) & 1);

    uint[4] occlusion = uint[4](
        (second_portion     ) & mask2,
        (second_portion >> 2) & mask2,
        (second_portion >> 4) & mask2,
        (second_portion >> 6) & mask2
    );

    uint texture_index = (second_portion >> 8);

    vec3 Size[4] = vec3[4](
        vec3(1    ,height,width ),
        vec3(width,1     ,height),
        vec3(width,height,1     ),
        vec3(1,1,1)
    );

    bool is_forward_fixed = (int(FaceType) == 2 || int(FaceType) == 1) ? is_forward : !is_forward;

    uint vertex_index = Indices[ is_forward_fixed ? (5 - (id % 6)) : (id % 6)];

    vec3 vertex_position = Vertices[int(FaceType) * 4 + vertex_index];
    vec3 pos = vertex_position * Size[int(FaceType)] + (vec3(x,y,z) + vec3(world_position) * 64);

    vec4 viewPos = player_camera_view_matrix * player_camera_model_matrix * vec4(pos, 1.0);
    FragPos = viewPos.xyz;
    gl_Position = player_camera_projection_matrix * viewPos;

    Normal = Normals[int(FaceType) * 2 + int(!is_forward)];

    vec2 TextCoordList[4] = vec2[4](
        vec2(width, height),
        vec2(height, width),
        vec2(width, height),
        vec2(width, height)
    );

    TexCoords = NormalizedTexCoords[vertex_index] * TextCoordList[int(FaceType)];
    TexIndex = texture_index;

    Occlusion = occlusion[vertex_index];
}
