struct BVHNode {
    float minX, minY, minZ;
    int   leftFirst;  // leaf: first tri index, inner: left child
    float maxX, maxY, maxZ;
    int   count;      // >0 = leaf with count tris, 0 = inner node
};

struct HitInfo {
    vec3  position;
    vec3  normal;
    vec2  barycentrics;
    float distance;
    bool  hit;
};

// SSBO definitions for BVH traversal
layout(std430, binding = 3) readonly buffer BVHNodes {
    BVHNode nodes[];
};

layout(std430, binding = 4) readonly buffer Vertices {
    vec4 vertices[]; // xyz position, w unused
};

layout(std430, binding = 5) readonly buffer Indices {
    uint indices[]; // triangle index buffer, 3 indices per triangle
};

// Requires intersection.glsl for Ray, IntersectAABB, IntersectTriangle
// Ensure intersection.glsl is included before this file.

// Trace a ray and return true on the first hit (useful for shadow/AO rays)
bool TraceAnyHit(vec3 origin, vec3 direction, float maxDist) {
    Ray ray;
    ray.origin = origin;
    ray.direction = direction;
    ray.tMin = 0.001;
    ray.tMax = maxDist;

    int stack[32];
    int stackPtr = 0;
    
    // Start at root node
    stack[stackPtr++] = 0;

    while (stackPtr > 0) {
        int nodeIdx = stack[--stackPtr];
        BVHNode node = nodes[nodeIdx];
        
        float tNear, tFar;
        if (!IntersectAABB(ray, vec3(node.minX, node.minY, node.minZ), vec3(node.maxX, node.maxY, node.maxZ), tNear, tFar))
            continue;

        if (node.count > 0) { // Leaf node
            int startIdx = node.leftFirst * 3;
            for (int i = 0; i < node.count; ++i) {
                uint i0 = indices[startIdx + i * 3 + 0];
                uint i1 = indices[startIdx + i * 3 + 1];
                uint i2 = indices[startIdx + i * 3 + 2];
                
                vec3 v0 = vertices[i0].xyz;
                vec3 v1 = vertices[i1].xyz;
                vec3 v2 = vertices[i2].xyz;
                
                float t, u, v;
                if (IntersectTriangle(ray, v0, v1, v2, t, u, v)) {
                    return true;
                }
            }
        } else { // Inner node
            // Push children. We could sort by distance, but for AnyHit it just needs to find one
            stack[stackPtr++] = node.leftFirst;
            stack[stackPtr++] = node.leftFirst + 1;
        }
    }
    
    return false;
}

// Trace a ray and find the closest intersection
HitInfo TraceClosestHit(vec3 origin, vec3 direction, float maxDist) {
    HitInfo hit;
    hit.hit = false;
    hit.distance = maxDist;
    
    Ray ray;
    ray.origin = origin;
    ray.direction = direction;
    ray.tMin = 0.001;
    ray.tMax = maxDist;

    int stack[32];
    int stackPtr = 0;
    
    // Start at root node
    stack[stackPtr++] = 0;

    while (stackPtr > 0) {
        int nodeIdx = stack[--stackPtr];
        BVHNode node = nodes[nodeIdx];
        
        float tNear, tFar;
        if (!IntersectAABB(ray, vec3(node.minX, node.minY, node.minZ), vec3(node.maxX, node.maxY, node.maxZ), tNear, tFar))
            continue;

        if (node.count > 0) { // Leaf node
            int startIdx = node.leftFirst * 3;
            for (int i = 0; i < node.count; ++i) {
                uint i0 = indices[startIdx + i * 3 + 0];
                uint i1 = indices[startIdx + i * 3 + 1];
                uint i2 = indices[startIdx + i * 3 + 2];
                
                vec3 v0 = vertices[i0].xyz;
                vec3 v1 = vertices[i1].xyz;
                vec3 v2 = vertices[i2].xyz;
                
                float t, u, v;
                if (IntersectTriangle(ray, v0, v1, v2, t, u, v)) {
                    hit.hit = true;
                    hit.distance = t;
                    hit.position = ray.origin + ray.direction * t;
                    hit.barycentrics = vec2(u, v);
                    // Compute geometric normal
                    hit.normal = normalize(cross(v1 - v0, v2 - v0));
                    
                    // Update max distance to cull further away AABBs
                    ray.tMax = t; 
                }
            }
        } else { // Inner node
            int leftIdx = node.leftFirst;
            int rightIdx = leftIdx + 1;
            
            BVHNode leftNode = nodes[leftIdx];
            BVHNode rightNode = nodes[rightIdx];
            
            float tNearL, tFarL, tNearR, tFarR;
            bool hitL = IntersectAABB(ray, vec3(leftNode.minX, leftNode.minY, leftNode.minZ), vec3(leftNode.maxX, leftNode.maxY, leftNode.maxZ), tNearL, tFarL);
            bool hitR = IntersectAABB(ray, vec3(rightNode.minX, rightNode.minY, rightNode.minZ), vec3(rightNode.maxX, rightNode.maxY, rightNode.maxZ), tNearR, tFarR);
            
            if (hitL && hitR) {
                if (tNearL < tNearR) {
                    stack[stackPtr++] = rightIdx;
                    stack[stackPtr++] = leftIdx;
                } else {
                    stack[stackPtr++] = leftIdx;
                    stack[stackPtr++] = rightIdx;
                }
            } else if (hitL) {
                stack[stackPtr++] = leftIdx;
            } else if (hitR) {
                stack[stackPtr++] = rightIdx;
            }
        }
    }
    
    return hit;
}
