#version 450

// Hash function to get pseudo-random gradients
vec3 randomGradient(int seed) {
    // Simple hash to get a random gradient
    return normalize(vec3(
        fract(sin(float(seed) * 123.45) * 678.9),
        fract(sin(float(seed) * 987.65) * 543.21),
        fract(sin(float(seed) * 456.78) * 123.45)
    )) * 2.0 - 1.0; // Normalize to range [-1, 1]
}

// Linear interpolation
float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// Fade function for smooth interpolation
float fade(float t) {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

// 3D Perlin noise function
float perlinNoise3D(vec3 pos) {
    // Floor position to find cell corners
    ivec3 cell = ivec3(floor(pos));
    vec3 localPos = pos - vec3(cell); // Local position within cell

    // Fade curves for interpolation
    vec3 fadeXYZ = vec3(fade(localPos.x), fade(localPos.y), fade(localPos.z));

    // Hash the corner coordinates of the cell to get gradients
    int hash000 = cell.x + cell.y * 57 + cell.z * 113;
    int hash100 = (cell.x + 1) + cell.y * 57 + cell.z * 113;
    int hash010 = cell.x + (cell.y + 1) * 57 + cell.z * 113;
    int hash110 = (cell.x + 1) + (cell.y + 1) * 57 + cell.z * 113;
    int hash001 = cell.x + cell.y * 57 + (cell.z + 1) * 113;
    int hash101 = (cell.x + 1) + cell.y * 57 + (cell.z + 1) * 113;
    int hash011 = cell.x + (cell.y + 1) * 57 + (cell.z + 1) * 113;
    int hash111 = (cell.x + 1) + (cell.y + 1) * 57 + (cell.z + 1) * 113;

    // Compute dot products of the distance vectors with the gradient vectors
    vec3 g000 = randomGradient(hash000);
    vec3 g100 = randomGradient(hash100);
    vec3 g010 = randomGradient(hash010);
    vec3 g110 = randomGradient(hash110);
    vec3 g001 = randomGradient(hash001);
    vec3 g101 = randomGradient(hash101);
    vec3 g011 = randomGradient(hash011);
    vec3 g111 = randomGradient(hash111);

    // Distance vectors from each corner
    vec3 d000 = localPos - vec3(0.0, 0.0, 0.0);
    vec3 d100 = localPos - vec3(1.0, 0.0, 0.0);
    vec3 d010 = localPos - vec3(0.0, 1.0, 0.0);
    vec3 d110 = localPos - vec3(1.0, 1.0, 0.0);
    vec3 d001 = localPos - vec3(0.0, 0.0, 1.0);
    vec3 d101 = localPos - vec3(1.0, 0.0, 1.0);
    vec3 d011 = localPos - vec3(0.0, 1.0, 1.0);
    vec3 d111 = localPos - vec3(1.0, 1.0, 1.0);

    // Dot products for each corner
    float dot000 = dot(g000, d000);
    float dot100 = dot(g100, d100);
    float dot010 = dot(g010, d010);
    float dot110 = dot(g110, d110);
    float dot001 = dot(g001, d001);
    float dot101 = dot(g101, d101);
    float dot011 = dot(g011, d011);
    float dot111 = dot(g111, d111);

    // Interpolate along x, y, and z
    float xInterp0 = lerp(dot000, dot100, fadeXYZ.x);
    float xInterp1 = lerp(dot010, dot110, fadeXYZ.x);
    float xInterp2 = lerp(dot001, dot101, fadeXYZ.x);
    float xInterp3 = lerp(dot011, dot111, fadeXYZ.x);

    float yInterp0 = lerp(xInterp0, xInterp1, fadeXYZ.y);
    float yInterp1 = lerp(xInterp2, xInterp3, fadeXYZ.y);

    return lerp(yInterp0, yInterp1, fadeXYZ.z); // Final interpolated noise value
}

float terrainHeight(vec3 position) {
    float height = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    int octaves = 4;

    for (int i = 0; i < octaves; i++) {
        height += perlinNoise3D(position * frequency) * amplitude;
        amplitude *= 0.5; // Reduce amplitude for each octave
        frequency *= 2.0;  // Increase frequency for each octave
    }

    return height;
}

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer TerrainData {
    uint data[];
};

void main() {
    // Each work group handles 32 blocks
    uint groupIndex = gl_WorkGroupID.x;
    uint bitIndex = gl_LocalInvocationID.x; // Thread-specific bit position (0 to 31)

    // Calculate the array index and bit mask for this work group's 32-bit uint
    uint arrayIndex = groupIndex;
    uint bitMask = 1u << bitIndex;

    // Determine if this block (bit) should be set or cleared
    bool isActive = terrainHeight(gl_GlobalInvocationID) > 0.5;

    // Set or clear the bit within the uint
    if (isActive) {
        atomicOr(data[arrayIndex], bitMask);  // Set the bit
    } else {
        atomicAnd(data[arrayIndex], ~bitMask); // Clear the bit
    }
}