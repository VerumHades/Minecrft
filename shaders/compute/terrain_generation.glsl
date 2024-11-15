#version 450

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer TerrainData {
    uint data[];
};

float random (vec3 st) {
    return fract(sin(dot(st.xyz,
                         vec3(12.9898,78.233,154.214)))*43758.5453123);
}

vec2 random2(vec2 st){
    st = vec2( dot(st,vec2(127.1,311.7)),
              dot(st,vec2(269.5,183.3)) );
    
    return -1.0 + 2.0*fract(sin(st)*43758.5453123);
}

// Gradient Noise by Inigo Quilez - iq/2013
// https://www.shadertoy.com/view/XdXGW8
float noise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    vec2 u = f*f*(3.0-2.0*f);

    return mix( mix( dot( random2(i + vec2(0.0,0.0) ), f - vec2(0.0,0.0) ),
                     dot( random2(i + vec2(1.0,0.0) ), f - vec2(1.0,0.0) ), u.x),
                mix( dot( random2(i + vec2(0.0,1.0) ), f - vec2(0.0,1.0) ),
                     dot( random2(i + vec2(1.0,1.0) ), f - vec2(1.0,1.0) ), u.x), u.y);
}

uniform vec3 worldPosition;

void main() {
    // Each work group handles 32 blocks
    uint groupIndex = gl_WorkGroupID.x + gl_WorkGroupID.y * 2 + gl_WorkGroupID.z * 64 * 2;
    uint bitIndex = gl_LocalInvocationID.x; // Thread-specific bit position (0 to 31)

    // Calculate the array index and bit mask for this work group's 32-bit uint
    uint arrayIndex = groupIndex;
    uint bitMask = 1u << bitIndex;

    vec3 position = gl_GlobalInvocationID.xzy + worldPosition;
    // Determine if this block (bit) should be set or cleared
    bool isActive = position.y < -1;

    // Set or clear the bit within the uint
    if (isActive) {
        atomicOr(data[arrayIndex], bitMask);  // Set the bit
    } else {
        atomicAnd(data[arrayIndex], ~bitMask); // Clear the bit
    }
}