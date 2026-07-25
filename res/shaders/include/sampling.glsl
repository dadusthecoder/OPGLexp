#define PI 3.14159265359

// Importance sample a cosine-weighted hemisphere
vec3 CosineWeightedHemisphere(vec3 normal, vec2 xi) {
    float r = sqrt(xi.x);
    float theta = 2.0 * PI * xi.y;
    
    vec3 s = vec3(r * cos(theta), r * sin(theta), sqrt(max(0.0, 1.0 - xi.x)));
    
    // Create an orthonormal basis around the normal
    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);
    
    return tangent * s.x + bitangent * s.y + normal * s.z;
}

// Uniformly sample a sphere (useful for DDGI probe rays)
vec3 UniformSphere(vec2 xi) {
    float z = 1.0 - 2.0 * xi.x;
    float r = sqrt(max(0.0, 1.0 - z * z));
    float phi = 2.0 * PI * xi.y;
    return vec3(r * cos(phi), r * sin(phi), z);
}

// Generate deterministic points on a sphere (useful for DDGI)
vec2 SphericalFibonacci(int index, int totalSamples) {
    float b = (sqrt(5.0) * 0.5 + 0.5);
    float phi = 2.0 * PI * fract(float(index) / b);
    float cosTheta = 1.0 - (2.0 * float(index) + 1.0) / float(totalSamples);
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    return vec2(phi, cosTheta);
}

// Reverses the bits of a 32-bit integer for Van der Corput sequence
float RadicalInverse(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// Generate a Hammersley sequence point
vec2 Hammersley(uint i, uint N) {
    return vec2(float(i) / float(N), RadicalInverse(i));
}

// Sign function that doesn't return 0
vec2 SignNotZero(vec2 v) {
    return vec2((v.x >= 0.0) ? 1.0 : -1.0, (v.y >= 0.0) ? 1.0 : -1.0);
}

// Octahedral encoding for storing unit vectors in 2D (useful for DDGI)
vec2 OctahedralEncode(vec3 dir) {
    // Project the sphere onto the octahedron, and then onto the xy plane
    vec2 p = dir.xy * (1.0 / (abs(dir.x) + abs(dir.y) + abs(dir.z)));
    // Reflect the folds of the lower hemisphere over the diagonals
    return (dir.z <= 0.0) ? ((1.0 - abs(p.yx)) * SignNotZero(p)) : p;
}

// Decode octahedral encoding back to a unit vector
vec3 OctahedralDecode(vec2 oct) {
    vec3 v = vec3(oct.x, oct.y, 1.0 - abs(oct.x) - abs(oct.y));
    if (v.z < 0.0) {
        v.xy = (1.0 - abs(v.yx)) * SignNotZero(v.xy);
    }
    return normalize(v);
}

// A simple hash function for integer randomization
uint WangHash(uint seed) {
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15u);
    return seed;
}

// Pseudo-random noise based on pixel coordinates and frame number
float BlueNoiseScalar(ivec2 pixel, int frame, sampler2D noiseTex) {
    // Typically you'd use a blue noise texture. This is a simplified fetch.
    ivec2 texSize = textureSize(noiseTex, 0);
    ivec2 coord = (pixel + ivec2(WangHash(uint(frame)) % uint(texSize.x), WangHash(uint(frame) + 1u) % uint(texSize.y))) % texSize;
    return texelFetch(noiseTex, coord, 0).r;
}
