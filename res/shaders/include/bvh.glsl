#ifndef BVH_GLSL
#define BVH_GLSL

// Must match CPU BVHNode struct (32 bytes)
struct BVHNode {
    float minX, minY, minZ;
    int   leftFirst;  // leaf: first tri idx; inner: left child idx
    float maxX, maxY, maxZ;
    int   triCount;   // >0 = leaf, 0 = inner node
};

struct HitInfo {
    vec3  position;
    vec3  normal;
    float distance;
    bool  hit;
};

// SSBOs bound by BVHPass::Bind()
layout(std430, binding = 6) readonly buffer BVHBuffer    { BVHNode bvhNodes[]; };
layout(std430, binding = 7) readonly buffer TriBuffer    { vec4    bvhTris[]; };  // 3 vec4 per tri (v0,v1,v2), then normal

// Slab test AABB intersection
bool IntersectAABB(vec3 ro, vec3 rdInv, vec3 bMin, vec3 bMax, float tMin, float tMax) {
    vec3 t0 = (bMin - ro) * rdInv;
    vec3 t1 = (bMax - ro) * rdInv;
    vec3 tNear = min(t0, t1);
    vec3 tFar  = max(t0, t1);
    float tN = max(max(tNear.x, tNear.y), max(tNear.z, tMin));
    float tF = min(min(tFar.x,  tFar.y),  min(tFar.z,  tMax));
    return tN <= tF;
}

// Möller-Trumbore ray-triangle intersection
bool IntersectTriangle(vec3 ro, vec3 rd, vec3 v0, vec3 v1, vec3 v2, out float t, out vec3 bary) {
    vec3 e1 = v1 - v0;
    vec3 e2 = v2 - v0;
    vec3 h  = cross(rd, e2);
    float a = dot(e1, h);
    if (abs(a) < 1e-8) return false;
    float f = 1.0 / a;
    vec3 s = ro - v0;
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0) return false;
    vec3 q = cross(s, e1);
    float v = f * dot(rd, q);
    if (v < 0.0 || u + v > 1.0) return false;
    t = f * dot(e2, q);
    bary = vec3(1.0 - u - v, u, v);
    return t > 1e-5;
}

// Any-hit traversal (for shadows / AO)
bool TraceAnyHit(vec3 ro, vec3 rd, float maxDist) {
    vec3 rdInv = 1.0 / (rd + vec3(1e-30));
    int stack[32];
    int stackTop = 0;
    stack[stackTop++] = 0;

    while (stackTop > 0) {
        int nodeIdx = stack[--stackTop];
        BVHNode node = bvhNodes[nodeIdx];

        vec3 bMin = vec3(node.minX, node.minY, node.minZ);
        vec3 bMax = vec3(node.maxX, node.maxY, node.maxZ);
        if (!IntersectAABB(ro, rdInv, bMin, bMax, 0.0, maxDist)) continue;

        if (node.triCount > 0) {  // Leaf
            for (int i = 0; i < node.triCount; i++) {
                int base = (node.leftFirst + i) * 4;
                vec3 v0 = bvhTris[base+0].xyz;
                vec3 v1 = bvhTris[base+1].xyz;
                vec3 v2 = bvhTris[base+2].xyz;
                float t; vec3 bary;
                if (IntersectTriangle(ro, rd, v0, v1, v2, t, bary) && t < maxDist)
                    return true;
            }
        } else {
            stack[stackTop++] = node.leftFirst;      // left
            stack[stackTop++] = node.leftFirst + 1;  // right
        }
    }
    return false;
}

// Closest-hit traversal
HitInfo TraceClosestHit(vec3 ro, vec3 rd, float maxDist) {
    HitInfo result;
    result.hit = false;
    result.distance = maxDist;

    vec3 rdInv = 1.0 / (rd + vec3(1e-30));
    int stack[32];
    int stackTop = 0;
    stack[stackTop++] = 0;

    while (stackTop > 0) {
        int nodeIdx = stack[--stackTop];
        BVHNode node = bvhNodes[nodeIdx];

        vec3 bMin = vec3(node.minX, node.minY, node.minZ);
        vec3 bMax = vec3(node.maxX, node.maxY, node.maxZ);
        if (!IntersectAABB(ro, rdInv, bMin, bMax, 0.0, result.distance)) continue;

        if (node.triCount > 0) {
            for (int i = 0; i < node.triCount; i++) {
                int base = (node.leftFirst + i) * 4;
                vec3 v0 = bvhTris[base+0].xyz;
                vec3 v1 = bvhTris[base+1].xyz;
                vec3 v2 = bvhTris[base+2].xyz;
                vec3 normal = bvhTris[base+3].xyz;
                float t; vec3 bary;
                if (IntersectTriangle(ro, rd, v0, v1, v2, t, bary) && t < result.distance) {
                    result.hit = true;
                    result.distance = t;
                    result.position = ro + rd * t;
                    result.normal = normalize(normal);
                }
            }
        } else {
            stack[stackTop++] = node.leftFirst;
            stack[stackTop++] = node.leftFirst + 1;
        }
    }
    return result;
}

#endif
