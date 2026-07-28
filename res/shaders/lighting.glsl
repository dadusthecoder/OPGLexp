#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

out vec2 v_TexCoord;

void main() {
    v_TexCoord = a_TexCoord;
    gl_Position = vec4(a_Position, 1.0);
}

#type fragment
#version 460 core

layout(location = 0) out vec4 o_Color;

in vec2 v_TexCoord;

#include "include/common.glsl"
#include "include/brdf.glsl"

layout(binding = 0) uniform sampler2D u_gAlbedo;
layout(binding = 1) uniform sampler2D u_gNormal;
layout(binding = 2) uniform sampler2D u_gPBR;
layout(binding = 3) uniform sampler2D u_gDepth;

layout(binding = 4) uniform samplerCube u_IrradianceMap;
layout(binding = 5) uniform samplerCube u_PrefilterMap;
layout(binding = 6) uniform sampler2D u_BrdfLut;
layout(binding = 7) uniform sampler2D u_AOTexture;
layout(binding = 8) uniform sampler2D u_DDGIIrradiance;

struct Light {
    vec3 Position;
    int  Type;       // 0=directional, 1=point
    vec3 Direction;
    float Radius;
    vec3 Color;
    float Intensity;
};

uniform Light u_Lights[16];
uniform int u_LightCount;

uniform vec3 u_CameraPos;
uniform mat4 u_InvViewProjection;

uniform ivec3 u_DDGIProbeGridSize;
uniform vec3 u_DDGIProbeOrigin;
uniform vec3 u_DDGIProbeSpacing;

vec3 SampleDDGI(vec3 worldPos, vec3 normal) {
    vec3 gridPos = (worldPos - u_DDGIProbeOrigin) / u_DDGIProbeSpacing;
    ivec3 baseProbe = clamp(ivec3(gridPos), ivec3(0), u_DDGIProbeGridSize - 2);
    vec3 alpha = fract(gridPos);
    
    vec3 irradiance = vec3(0.0);
    float totalWeight = 0.0;
    
    for (int z = 0; z <= 1; z++) {
    for (int y = 0; y <= 1; y++) {
    for (int x = 0; x <= 1; x++) {
        ivec3 probeIdx = clamp(baseProbe + ivec3(x,y,z), ivec3(0), u_DDGIProbeGridSize-1);
        int linearIdx = probeIdx.x + probeIdx.y * u_DDGIProbeGridSize.x + probeIdx.z * u_DDGIProbeGridSize.x * u_DDGIProbeGridSize.y;
        
        // Trilinear weight
        vec3 triW = mix(1.0 - alpha, alpha, vec3(x, y, z));
        float w = triW.x * triW.y * triW.z;
        
        // Atlas UV for this probe's irradiance (8x8 texels + 2px border per probe)
        int stride = 10;  // 8 + 2 border
        int probesPerRow = u_DDGIProbeGridSize.x;
        int atlasX = (linearIdx % probesPerRow) * stride + 1;
        int atlasY = (linearIdx / probesPerRow) * stride + 1;
        
        // Sample using octahedral direction
        vec2 octUV = OctEncode(normal) * float(8) / textureSize(u_DDGIIrradiance, 0);
        vec2 probeBaseUV = (vec2(atlasX, atlasY) + 0.5) / vec2(textureSize(u_DDGIIrradiance, 0));
        vec2 sampleUV = probeBaseUV + octUV;
        
        irradiance += texture(u_DDGIIrradiance, sampleUV).rgb * w;
        totalWeight += w;
    }}}
    
    return (totalWeight > 0.001) ? irradiance / totalWeight : vec3(0.0);
}

void main() {
    float depth = texture(u_gDepth, v_TexCoord).r;
    if (depth == 1.0) {
        o_Color = vec4(0.0, 0.0, 0.0, 1.0); // or skybox later
        return;
    }

    vec4 albedoData = texture(u_gAlbedo, v_TexCoord);
    vec3 albedo = albedoData.rgb;
    float isEmissive = albedoData.a;

    vec2 normalData = texture(u_gNormal, v_TexCoord).rg;
    vec3 N = OctDecode(normalData);

    vec4 pbrData = texture(u_gPBR, v_TexCoord);
    float metallic = pbrData.r;
    float roughness = max(pbrData.g, 0.04);
    float ao = pbrData.b;
    float emissiveStrength = pbrData.a;

    vec3 worldPos = ReconstructWorldPos(v_TexCoord, depth, u_InvViewProjection);
    vec3 V = SafeNormalize(u_CameraPos - worldPos);

    vec3 Lo = vec3(0.0);
    
    for (int i = 0; i < u_LightCount; ++i) {
        vec3 L;
        float attenuation = 1.0;
        
        if (u_Lights[i].Type == 0) {
            L = normalize(-u_Lights[i].Direction);
        } else {
            L = u_Lights[i].Position - worldPos;
            float dist = length(L);
            L = normalize(L);
            
            float d = dist / max(u_Lights[i].Radius, 0.001);
            float d2 = d * d;
            float d4 = d2 * d2;
            float falloff = clamp(1.0 - d4, 0.0, 1.0);
            attenuation = (falloff * falloff) / (dist * dist + 1.0);
        }
        
        vec3 lightColor = u_Lights[i].Color * u_Lights[i].Intensity * attenuation;
        Lo += DirectLight(N, V, L, albedo, metallic, roughness, lightColor);
    }

    // IBL
    vec3 R = reflect(-V, N);
    
    vec3 irradiance = vec3(0.0);
    if (u_DDGIProbeGridSize.x > 0) {
        irradiance = SampleDDGI(worldPos, N);
    } else {
        irradiance = texture(u_IrradianceMap, N).rgb;
    }
    
    vec3 diffuseIBL = irradiance * albedo * (1.0 - metallic);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(u_PrefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(u_BrdfLut, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specularIBL = prefilteredColor * (F0 * brdf.x + brdf.y);

    float rtao = texture(u_AOTexture, v_TexCoord).r;
    vec3 ambient = (diffuseIBL + specularIBL) * ao * rtao;

    vec3 color = ambient + Lo;

    if (isEmissive > 0.5) {
        color += albedo * emissiveStrength;
    }

    o_Color = vec4(color, 1.0);
}
