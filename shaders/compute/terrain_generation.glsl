#version 450

layout(local_size_x = 1, local_size_y = 1, local_size_z = 32) in;

layout(std430, binding = 0) buffer TerrainData {
    uint data[];
};

vec3 random(vec3 st){
    st = vec3(
        dot(st,vec3(127.1,311.7,84.1)),
        dot(st,vec3(269.5,183.3,45.4)),
        dot(st,vec3(28.5,153.3,965.4))
    );
    return -1.0 + 2.0*fract(sin(st)*43758.5453123);
}

float noise3D(vec3 position){
    vec3 grid_position = floor(position);
    vec3 ratio = fract(position);

    vec3 weight_vector1 = random(grid_position + vec3(0,1,0));
    vec3 weight_vector2 = random(grid_position + vec3(1,1,0));
    vec3 weight_vector3 = random(grid_position + vec3(1,0,0));
    vec3 weight_vector4 = random(grid_position + vec3(0,0,0));

    vec3 weight_vector5 = random(grid_position + vec3(0,1,1));
    vec3 weight_vector6 = random(grid_position + vec3(1,1,1));
    vec3 weight_vector7 = random(grid_position + vec3(1,0,1));
    vec3 weight_vector8 = random(grid_position + vec3(0,0,1));

    vec3 distance_vector1 =  position - (grid_position + vec3(0,1,0));
    vec3 distance_vector2 =  position - (grid_position + vec3(1,1,0));
    vec3 distance_vector3 =  position - (grid_position + vec3(1,0,0));
    vec3 distance_vector4 =  position - (grid_position + vec3(0,0,0));

    vec3 distance_vector5 =  position - (grid_position + vec3(0,1,1));
    vec3 distance_vector6 =  position - (grid_position + vec3(1,1,1));
    vec3 distance_vector7 =  position - (grid_position + vec3(1,0,1));
    vec3 distance_vector8 =  position - (grid_position + vec3(0,0,1));

    float influence0 = dot(weight_vector1, distance_vector1);
    float influence1 = dot(weight_vector2, distance_vector2);
    float influence2 = dot(weight_vector3, distance_vector3);
    float influence3 = dot(weight_vector4, distance_vector4);

    float influence4 = dot(weight_vector5, distance_vector5);
    float influence5 = dot(weight_vector6, distance_vector6);
    float influence6 = dot(weight_vector7, distance_vector7);
    float influence7 = dot(weight_vector8, distance_vector8);

    float influence_top_front = mix(influence0, influence1, ratio.x);
    float influence_top_back  = mix(influence4, influence5, ratio.x);
    
    float influence_top = mix(influence_top_front, influence_top_back, ratio.z); 

    float influence_bottom_front = mix(influence3, influence2, ratio.x);
    float influence_bottom_back  = mix(influence7, influence6, ratio.x);
    
    float influence_bottom = mix(influence_bottom_front, influence_bottom_back, ratio.z); 

    float value = mix(influence_top, influence_bottom, ratio.y);

    return value;
}

uniform vec3 worldPosition;

void main() {
    // Each work group handles 32 blocks
    uint groupIndex = (1 - gl_WorkGroupID.z) + gl_WorkGroupID.x * 2 + gl_WorkGroupID.y * 64 * 2;
    uint bitIndex = gl_LocalInvocationID.z; // Thread-specific bit position (0 to 31)

    // Calculate the array index and bit mask for this work group's 32-bit uint
    uint arrayIndex = groupIndex;
    uint bitMask = 1u << (31 - bitIndex);

    vec3 position = gl_GlobalInvocationID.xyz + worldPosition;
    // Determine if this block (bit) should be set or cleared
    bool isActive = position.y < ((sin(position.x / 64) + cos(position.z / 64)) * 10);//position.y < 0.0;

    // Set or clear the bit within the uint
    if (isActive) {
        atomicOr(data[arrayIndex], bitMask);  // Set the bit
    } else {
        atomicAnd(data[arrayIndex], ~bitMask); // Clear the bit
    }
}