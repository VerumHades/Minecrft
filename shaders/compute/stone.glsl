#version 450

layout(local_size_x = 1, local_size_y = 1, local_size_z = 32) in;

layout(std430, binding = 0) buffer TerrainData {
    uint data[];
};

uniform vec3 worldPosition;
uniform sampler2D noiseTexture;

void main() {
    // Each work group handles 32 blocks
    uint groupIndex = (1 - gl_WorkGroupID.z) + gl_WorkGroupID.x * 2 + gl_WorkGroupID.y * 64 * 2;
    uint bitIndex = gl_LocalInvocationID.z; // Thread-specific bit position (0 to 31)

    // Calculate the array index and bit mask for this work group's 32-bit uint
    uint arrayIndex = groupIndex;
    uint bitMask = 1u << (31 - bitIndex);

    vec3 position = gl_GlobalInvocationID.xyz + worldPosition;

    //float current_height = ((sin(position.x / 64) + cos(position.z / 64)) * 10);
    float current_height = texture(noiseTexture, (position.xz + 512) / 1024.0f).r * 255;
    // Determine if this block (bit) should be set or cleared
    bool isActive = (position.y <= current_height - 3);//position.y < 0.0;

    // Set or clear the bit within the uint
    if (isActive) {
        atomicOr(data[arrayIndex], bitMask);  // Set the bit
    } else {
        atomicAnd(data[arrayIndex], ~bitMask); // Clear the bit
    }
}