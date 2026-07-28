#ifndef SAMPLING_GLSL
#define SAMPLING_GLSL

// Van der Corput low-discrepancy sequence
float RadicalInverse(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

// Halton low-discrepancy 2D sequence (base 2,3)
vec2 Hammersley(uint i, uint N) {
    return vec2(float(i) / float(N), RadicalInverse(i));
}

// Halton base-b
float Halton(uint index, uint base) {
    float f = 1.0, r = 0.0;
    uint i = index;
    while (i > 0u) { f /= float(base); r += f * float(i % base); i /= base; }
    return r;
}

vec2 Halton2D(uint index) {
    return vec2(Halton(index, 2u), Halton(index, 3u));
}

// Wang hash for pseudo-random
uint WangHash(uint seed) {
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed ^= seed >> 4u;
    seed *= 0x27d4eb2du;
    seed ^= seed >> 15u;
    return seed;
}

float WangHashFloat(uint seed) { return float(WangHash(seed)) / 4294967296.0; }

// Cosine-weighted hemisphere sample (tangent space Z-up)
vec3 CosineHemisphere(vec2 xi) {
    float r = sqrt(xi.x);
    float phi = TWO_PI * xi.y;
    return vec3(r * cos(phi), r * sin(phi), sqrt(max(0.0, 1.0 - xi.x)));
}

// Uniform hemisphere sample
vec3 UniformHemisphere(vec2 xi) {
    float z   = xi.x;
    float r   = sqrt(max(0.0, 1.0 - z*z));
    float phi = TWO_PI * xi.y;
    return vec3(r * cos(phi), r * sin(phi), z);
}

// Uniform sphere sample
vec3 UniformSphere(vec2 xi) {
    float z   = 1.0 - 2.0 * xi.x;
    float r   = sqrt(max(0.0, 1.0 - z*z));
    float phi = TWO_PI * xi.y;
    return vec3(r * cos(phi), r * sin(phi), z);
}

// Spherical Fibonacci (deterministic low-discrepancy sphere directions)
vec3 SphericalFibonacci(uint i, uint n) {
    float phi = TWO_PI * Halton(i, 2u);
    float cosTheta = 1.0 - (2.0 * float(i) + 1.0) / float(n);
    float sinTheta = sqrt(clamp(1.0 - cosTheta * cosTheta, 0.0, 1.0));
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

// Build orthonormal basis from normal (Frisvad method)
void BuildTBN(vec3 N, out vec3 T, out vec3 B) {
    if (abs(N.x) > 0.9)
        T = normalize(cross(vec3(0,1,0), N));
    else
        T = normalize(cross(vec3(1,0,0), N));
    B = cross(N, T);
}

// Transform direction from tangent-space to world-space
vec3 TangentToWorld(vec3 dir, vec3 N) {
    vec3 T, B;
    BuildTBN(N, T, B);
    return normalize(dir.x * T + dir.y * B + dir.z * N);
}

// GGX importance sample direction (for IBL prefilter)
vec3 ImportanceSampleGGX(vec2 xi, vec3 N, float roughness) {
    float a = roughness * roughness;
    float phi = TWO_PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a*a - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    vec3 H = vec3(cos(phi)*sinTheta, sin(phi)*sinTheta, cosTheta);
    return TangentToWorld(H, N);
}

// Octahedral encode/decode for DDGI atlas
vec2 OctahedralEncode(vec3 n) {
    n /= abs(n.x) + abs(n.y) + abs(n.z);
    if (n.z < 0.0) n.xy = (1.0 - abs(n.yx)) * sign(n.xy);
    return n.xy * 0.5 + 0.5;
}
vec3 OctahedralDecode(vec2 f) {
    f = f * 2.0 - 1.0;
    vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = clamp(-n.z, 0.0, 1.0);
    n.xy += n.xy >= 0.0 ? -t : t;
    return normalize(n);
}

#endif
