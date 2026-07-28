
#ifndef COMMON_GLSL
#define COMMON_GLSL

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;
const float INV_PI = 0.31830988618;
const float EPSILON = 1e-6;
const float FLT_MAX = 3.402823466e+38;

// sRGB linearization
vec3 SRGBToLinear(vec3 srgb) { return pow(srgb, vec3(2.2)); }
vec3 LinearToSRGB(vec3 lin)  { return pow(lin, vec3(1.0/2.2)); }

// Luminance
float Luminance(vec3 color) { return dot(color, vec3(0.2126, 0.7152, 0.0722)); }

// Safe normalize
vec3 SafeNormalize(vec3 v) { float l = length(v); return l > EPSILON ? v/l : vec3(0,1,0); }

// Reconstruct world position from depth buffer
vec3 ReconstructWorldPos(vec2 uv, float depth, mat4 invVP) {
    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 world = invVP * clip;
    return world.xyz / world.w;
}

// Oct-encode/decode normals [0,1] range
vec2 OctEncode(vec3 n) {
    n /= abs(n.x) + abs(n.y) + abs(n.z);
    if (n.z < 0.0) n.xy = (1.0 - abs(n.yx)) * sign(n.xy);
    return n.xy * 0.5 + 0.5;
}
vec3 OctDecode(vec2 f) {
    f = f * 2.0 - 1.0;
    vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = clamp(-n.z, 0.0, 1.0);
    n.x += n.x >= 0.0 ? -t : t;
    n.y += n.y >= 0.0 ? -t : t;
    return normalize(n);
}

#endif
