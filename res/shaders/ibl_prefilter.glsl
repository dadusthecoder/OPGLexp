#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;
out vec3 v_LocalPos;
uniform mat4 u_ViewProjection;

void main() {
    v_LocalPos = a_Position;
    gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 460 core

#include "include/common.glsl"
#include "include/sampling.glsl"

in vec3 v_LocalPos;
out vec4 o_Color;

uniform samplerCube u_EnvMap;
uniform float u_Roughness;

void main() {
    vec3 N = normalize(v_LocalPos);
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampleGGX(xi, N, u_Roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0) {
            prefilteredColor += texture(u_EnvMap, L).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    
    prefilteredColor = prefilteredColor / totalWeight;
    o_Color = vec4(prefilteredColor, 1.0);
}
