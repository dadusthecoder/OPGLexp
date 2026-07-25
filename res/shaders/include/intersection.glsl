struct Ray {
    vec3 origin;
    vec3 direction;
    float tMin;
    float tMax;
};

// Slab test for Ray-AABB intersection
bool IntersectAABB(Ray ray, vec3 bmin, vec3 bmax, out float tNear, out float tFar) {
    vec3 invDir = 1.0 / ray.direction;
    vec3 t0 = (bmin - ray.origin) * invDir;
    vec3 t1 = (bmax - ray.origin) * invDir;

    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);

    tNear = max(max(tmin.x, tmin.y), tmin.z);
    tFar = min(min(tmax.x, tmax.y), tmax.z);

    return tNear <= tFar && tFar >= ray.tMin && tNear <= ray.tMax;
}

// Möller-Trumbore algorithm for Ray-Triangle intersection
bool IntersectTriangle(Ray ray, vec3 v0, vec3 v1, vec3 v2, out float t, out float u, out float v) {
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 h = cross(ray.direction, edge2);
    float a = dot(edge1, h);

    // If a is near zero, ray is parallel to the triangle
    if (abs(a) < 0.0000001)
        return false;

    float f = 1.0 / a;
    vec3 s = ray.origin - v0;
    u = f * dot(s, h);

    if (u < 0.0 || u > 1.0)
        return false;

    vec3 q = cross(s, edge1);
    v = f * dot(ray.direction, q);

    if (v < 0.0 || u + v > 1.0)
        return false;

    // At this stage we can compute t to find out where the intersection point is on the line
    t = f * dot(edge2, q);

    // Ray intersection must be within [tMin, tMax]
    if (t > ray.tMin && t < ray.tMax)
        return true;

    return false;
}
