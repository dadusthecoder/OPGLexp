#shader Compute
#version 460 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D equirectangularMap;
layout(binding = 1, rgba16f) restrict writeonly uniform imageCube cubemap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

vec3 getCubeDir(ivec2 texCoord, int face, float width, float height)
{
    vec2 uv = vec2(texCoord.x / width, texCoord.y / height) * 2.0 - 1.0;
    
    // Default OpenGL cubemap face vectors
    vec3 dir = vec3(0.0);
    switch(face) {
        case 0: dir = vec3( 1.0, -uv.y, -uv.x); break; // POSITIVE_X
        case 1: dir = vec3(-1.0, -uv.y,  uv.x); break; // NEGATIVE_X
        case 2: dir = vec3( uv.x,  1.0,  uv.y); break; // POSITIVE_Y
        case 3: dir = vec3( uv.x, -1.0, -uv.y); break; // NEGATIVE_Y
        case 4: dir = vec3( uv.x, -uv.y,  1.0); break; // POSITIVE_Z
        case 5: dir = vec3(-uv.x, -uv.y, -1.0); break; // NEGATIVE_Z
    }
    return normalize(dir);
}

void main()
{
    ivec3 texCoord = ivec3(gl_GlobalInvocationID);
    
    ivec2 size = imageSize(cubemap);
    if(texCoord.x >= size.x || texCoord.y >= size.y) return;
    
    vec3 dir = getCubeDir(texCoord.xy, texCoord.z, float(size.x), float(size.y));
    
    vec2 uv = SampleSphericalMap(dir);
    vec4 color = texture(equirectangularMap, uv);
    
    imageStore(cubemap, texCoord, color);
}
