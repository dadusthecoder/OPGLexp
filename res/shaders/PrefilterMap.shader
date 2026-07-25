#shader Compute
#version 460 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0) uniform samplerCube environmentMap;
layout(binding = 1, rgba16f) restrict writeonly uniform imageCube prefilterMap;

uniform float roughness;

const float PI = 3.14159265359;

// Hammersley and GGX functions
float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness*roughness;
    
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
    
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

vec3 getCubeDir(ivec2 texCoord, int face, float width, float height)
{
    vec2 uv = vec2(texCoord.x / width, texCoord.y / height) * 2.0 - 1.0;
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
    ivec2 size = imageSize(prefilterMap);
    if(texCoord.x >= size.x || texCoord.y >= size.y) return;
    
    vec3 N = getCubeDir(texCoord.xy, texCoord.z, float(size.x), float(size.y));    
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    float totalWeight = 0.0;   
    vec3 prefilteredColor = vec3(0.0);     
    
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            prefilteredColor += texture(environmentMap, L).rgb * NdotL;
            totalWeight      += NdotL;
        }
    }
    prefilteredColor = prefilteredColor / totalWeight;

    imageStore(prefilterMap, texCoord, vec4(prefilteredColor, 1.0));
}
