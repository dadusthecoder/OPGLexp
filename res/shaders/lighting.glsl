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

#include "include/common.glsl"
#include "include/brdf.glsl"

layout(location = 0) out vec4 o_Color;

in vec2 v_TexCoord;

struct LightData {
    vec4 Position;
    vec4 Color;
    int Type;
    float Intensity;
    float Radius;
    float Falloff;
    vec4 Direction;
};

layout(std430, binding = 1) readonly buffer LightBuffer {
    LightData b_Lights[];
};

layout(std430, binding = 2) readonly buffer LightGridBuffer {
    uint b_LightGrid[];
};

layout(std430, binding = 3) readonly buffer LightIndexBuffer {
    uint b_LightIndices[];
};

uniform mat4 u_ViewMatrix;
uniform ivec3 u_GridSize;
uniform float u_ZNear;
uniform float u_ZFar;

layout(binding = 0) uniform sampler2D u_gAlbedo;
layout(binding = 1) uniform sampler2D u_gNormal;
layout(binding = 2) uniform sampler2D u_gPBR;
layout(binding = 3) uniform sampler2D u_gDepth;

layout(binding = 4) uniform samplerCube u_IrradianceMap;
layout(binding = 5) uniform samplerCube u_PrefilterMap;
layout(binding = 6) uniform sampler2D u_BrdfLut;
layout(binding = 7) uniform sampler2D u_AOTexture;
layout(binding = 8) uniform sampler2D u_DDGIIrradiance;
layout(binding = 9) uniform sampler2DArray u_ShadowCascades;
layout(binding = 10) uniform sampler2D u_DDGIDistance;
layout(binding = 11) uniform sampler2D u_RadianceAtlas;

layout(std430, binding = 10) readonly buffer DDGIProbeStateBuffer { vec4 b_DDGIProbeState[]; };


uniform mat4 u_LightSpaceMatrices[4];
uniform float u_CascadeSplits[4];
// SSBO lights used instead

uniform vec3 u_CameraPos;
uniform mat4 u_InvViewProjection;

uniform ivec3 u_DDGIProbeGridSize;
uniform vec3 u_DDGIProbeOrigin;
uniform vec3 u_DDGIProbeSpacing;
uniform int u_DDGIProbesPerRow;


uniform int u_EnableRTAO;
uniform int u_EnableDDGI;
uniform float u_DDGIIntensity;
uniform int u_EnableRTShadows;
uniform int u_EnableIBL;

uniform int u_EnableRC;
uniform int u_RCBaseProbeSpacing;
uniform int u_RCScreenWidth;
uniform int u_RCScreenHeight;
uniform float u_RCIntensity;

uniform int u_RCDebugCategory;
uniform int u_RCDebugMode;

// Simple HDR tonemap
vec3 Tonemap(vec3 color) {
    return color / (color + vec3(1.0));
}

vec3 SampleRadianceCascades(vec2 screenUV, vec3 N, vec3 V, out vec3 specularOut, float roughness) {
    ivec2 screenCoord = ivec2(screenUV * vec2(u_RCScreenWidth, u_RCScreenHeight));
    int spacing = u_RCBaseProbeSpacing;
    
    vec2 spatialCoord = (vec2(screenCoord) - float(spacing) / 2.0) / float(spacing);
    ivec2 spatialBase = ivec2(floor(spatialCoord));
    vec2 spatialFract = fract(spatialCoord);
    
    vec3 totalIrradiance = vec3(0.0);
    vec3 totalSpecular = vec3(0.0);
    vec3 R = reflect(-V, N);

    for (int sy = 0; sy <= 1; sy++) {
        for (int sx = 0; sx <= 1; sx++) {
            ivec2 probeNext = spatialBase + ivec2(sx, sy);
            probeNext.x = clamp(probeNext.x, 0, u_RCScreenWidth / spacing - 1);
            probeNext.y = clamp(probeNext.y, 0, u_RCScreenHeight / spacing - 1);
            
            float spatialW = (sx == 0 ? (1.0 - spatialFract.x) : spatialFract.x) *
                             (sy == 0 ? (1.0 - spatialFract.y) : spatialFract.y);
                             
            vec3 probeIrradiance = vec3(0.0);
            vec3 probeSpecular = vec3(0.0);
            float totalDiffW = 0.0;
            float totalSpecW = 0.0;
            
            for (int dy = 0; dy < spacing; dy++) {
                for (int dx = 0; dx < spacing; dx++) {
                    int texX = probeNext.x * spacing + dx;
                    int texY = probeNext.y * spacing + dy;
                    
                    vec3 radiance = texelFetch(u_RadianceAtlas, ivec2(texX, texY), 0).rgb;
                    
                    vec2 octUV = (vec2(dx, dy) + 0.5) / float(spacing);
                    vec3 dir = OctDecode(octUV * 2.0 - 1.0);
                    dir = normalize(dir);
                    
                    float ndotl = max(dot(N, dir), 0.0);
                    probeIrradiance += radiance * ndotl;
                    totalDiffW += ndotl;

                    float rdotv = max(dot(R, dir), 0.0);
                    float specPower = exp2(10.0 * (1.0 - roughness));
                    float specW = pow(rdotv, specPower);
                    probeSpecular += radiance * specW;
                    totalSpecW += specW;
                }
            }
            if (totalDiffW > 0.0) probeIrradiance /= totalDiffW;
            probeIrradiance *= PI;
            
            if (totalSpecW > 0.0) probeSpecular /= totalSpecW;
            
            totalIrradiance += probeIrradiance * spatialW;
            totalSpecular += probeSpecular * spatialW;
        }
    }
    
    specularOut = totalSpecular;
    return totalIrradiance;
}

vec3 SampleDDGI(vec3 worldPos, vec3 normal) {
    // Surface bias to prevent self-shadow / shadow acne
    vec3 V = SafeNormalize(u_CameraPos - worldPos);
    vec3 biasedPos = worldPos + (normal * 0.2 + V * 0.8) * length(u_DDGIProbeSpacing) * 0.1;

    vec3 gridPos = (biasedPos - u_DDGIProbeOrigin) / u_DDGIProbeSpacing;
    ivec3 baseProbe = clamp(ivec3(gridPos), ivec3(0), u_DDGIProbeGridSize - 2);
    vec3 alpha = fract(gridPos);

    int probesPerRow = u_DDGIProbesPerRow;
    vec3 irradiance = vec3(0.0);
    float totalWeight = 0.0;

    for (int z = 0; z <= 1; z++) {
    for (int y = 0; y <= 1; y++) {
    for (int x = 0; x <= 1; x++) {
        ivec3 probeIdx3 = clamp(baseProbe + ivec3(x, y, z), ivec3(0), u_DDGIProbeGridSize - 1);
        vec3 probeWorldPos = u_DDGIProbeOrigin + vec3(probeIdx3) * u_DDGIProbeSpacing;

        // Read probe state (relocation offset + active/dead)
        int linearIdx = probeIdx3.x + probeIdx3.y * u_DDGIProbeGridSize.x + probeIdx3.z * u_DDGIProbeGridSize.x * u_DDGIProbeGridSize.y;
        vec4 state = b_DDGIProbeState[linearIdx];
        probeWorldPos += state.xyz; // Apply relocation

        // Skip dead probes
        if (state.w < 0.5) continue;

        // --- Weight 1: Trilinear ---
        vec3 triW = mix(1.0 - alpha, alpha, vec3(x, y, z));
        float w = triW.x * triW.y * triW.z;

        // --- Weight 2: Backface rejection ---
        vec3 probeToSurface = normalize(biasedPos - probeWorldPos);
        float backfaceW = max(0.05, dot(probeToSurface, normal));
        w *= backfaceW;

        // --- Weight 3: Chebyshev visibility ---
        vec3 dirToProbe = probeWorldPos - biasedPos;
        float distToProbe = length(dirToProbe);
        dirToProbe /= max(distToProbe, EPSILON);

        // Atlas UV for distance texture (16x16 tiles, 14x14 inner)
        int atlasLinear = (probeIdx3.z * u_DDGIProbeGridSize.x + probeIdx3.x) + probeIdx3.y * probesPerRow;
        int tileX_d = atlasLinear % probesPerRow;
        int tileY_d = atlasLinear / probesPerRow;
        vec2 octUV_d = OctEncode(-dirToProbe);
        vec2 distTexelUV = (vec2(tileX_d * 16 + 1, tileY_d * 16 + 1) + octUV_d * 14.0) / vec2(textureSize(u_DDGIDistance, 0));
        vec2 moments = texture(u_DDGIDistance, distTexelUV).rg;

        float meanDist = moments.x;
        float meanDistSq = moments.y;

        if (distToProbe > meanDist) {
            float variance = max(meanDistSq - meanDist * meanDist, 0.0001);
            float diff = distToProbe - meanDist;
            float pMax = variance / (variance + diff * diff);
            float chebyshevW = max(pow(max((pMax - 0.25) / 0.75, 0.0), 3.0), 0.0);
            w *= chebyshevW;
        }

        // --- Sample irradiance ---
        int tileX_i = atlasLinear % probesPerRow;
        int tileY_i = atlasLinear / probesPerRow;
        vec2 octUV_i = OctEncode(normal);
        vec2 irrTexelUV = (vec2(tileX_i * 8 + 1, tileY_i * 8 + 1) + octUV_i * 6.0) / vec2(textureSize(u_DDGIIrradiance, 0));
        vec3 probeIrr = pow(texture(u_DDGIIrradiance, irrTexelUV).rgb, vec3(5.0)); // Gamma decode

        irradiance += probeIrr * w;
        totalWeight += w;
    }}}

    return (totalWeight > 0.001) ? irradiance / totalWeight : vec3(0.0);
}

float ShadowCalculation(vec3 worldPos, vec3 N, vec3 L) {
    float depthVS = abs((u_ViewMatrix * vec4(worldPos, 1.0)).z);
    int layer = -1;
    for (int i = 0; i < 4; ++i) {
        if (depthVS < u_CascadeSplits[i]) {
            layer = i;
            break;
        }
    }
    if (layer == -1) layer = 3;

    float NdotL = max(dot(N, L), 0.0);
    
    // Normal offset to push outside the shadow acne zone
    float offsetScale = 0.05 * (1.0 - NdotL);
    
    // Shift position along normal
    vec3 offsetWorldPos = worldPos + N * offsetScale;
    
    // Push slightly towards light to further prevent self-shadowing
    offsetWorldPos += L * 0.01;

    vec4 fragPosLightSpace = u_LightSpaceMatrices[layer] * vec4(offsetWorldPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.z > 1.0) {
        return 1.0;
    }

    float currentDepth = projCoords.z;
    // Keep bias very small because Z is already linear in ortho space
    float bias = max(0.001 * (1.0 - NdotL), 0.0005);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(u_ShadowCascades, 0));
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(u_ShadowCascades, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r; 
            shadow += currentDepth - bias > pcfDepth ? 0.0 : 1.0;
        }    
    }
    shadow /= 9.0;
    
    return shadow;
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

    // Flip normal towards camera for two-sided surfaces / backfacing geometry
    if (dot(N, V) < 0.0) {
        N = -N;
    }

    vec3 Lo = vec3(0.0);
    
    // Determine the cluster index for the current fragment
    vec4 viewPosHomogeneous = u_ViewMatrix * vec4(worldPos, 1.0);
    vec3 viewPos = viewPosHomogeneous.xyz / viewPosHomogeneous.w;

    uint zTile = 0;
    if (-viewPos.z >= u_ZNear) {
        zTile = uint(max(0.0, log2(-viewPos.z / u_ZNear) * float(u_GridSize.z) / log2(u_ZFar / u_ZNear)));
        zTile = min(zTile, u_GridSize.z - 1);
    }
    
    uint xTile = uint(gl_FragCoord.x * float(u_GridSize.x) / textureSize(u_gAlbedo, 0).x);
    uint yTile = uint(gl_FragCoord.y * float(u_GridSize.y) / textureSize(u_gAlbedo, 0).y);
    xTile = min(xTile, u_GridSize.x - 1);
    yTile = min(yTile, u_GridSize.y - 1);

    uint clusterIndex = xTile + yTile * u_GridSize.x + zTile * (u_GridSize.x * u_GridSize.y);
    
    uint lightOffset = b_LightGrid[clusterIndex * 2 + 0];
    uint lightCount  = b_LightGrid[clusterIndex * 2 + 1];

    for (uint i = 0; i < lightCount; ++i) {
        uint lightIdx = b_LightIndices[lightOffset + i];
        LightData light = b_Lights[lightIdx];
        
        vec3 L;
        float attenuation = 1.0;
        
        if (light.Type == 0) {
            L = normalize(-light.Direction.xyz);
        } else {
            L = light.Position.xyz - worldPos;
            float dist = length(L);
            L = normalize(L);
            
            float d = dist / max(light.Radius, 0.001);
            float d2 = d * d;
            float d4 = d2 * d2;
            float falloff = clamp(1.0 - d4, 0.0, 1.0);
            attenuation = (falloff * falloff) / (dist * dist + 1.0);
        }
        
        vec3 lightColor = light.Color.xyz * light.Intensity * attenuation;
        
        // Apply RT Shadows / CSM for directional light (Type 0)
        if (light.Type == 0 && u_EnableRTShadows != 0) {
            float shadow = ShadowCalculation(worldPos, N, L);
            lightColor *= shadow;
        }

        Lo += DirectLight(N, V, L, albedo, metallic, roughness, lightColor);
    }

    // IBL & Ambient
    vec3 R = reflect(-V, N);
    
    vec3 irradiance = vec3(0.0);
    vec3 rcSpecular = vec3(0.0);
    bool useRCSpecular = false;

    if (u_EnableRC != 0) {
        irradiance = SampleRadianceCascades(v_TexCoord, N, V, rcSpecular, roughness) * INV_PI * u_RCIntensity;
        useRCSpecular = true;
    } else if (u_EnableDDGI != 0 && u_DDGIProbeGridSize.x > 0) {
        // DDGI stores raw cosine-weighted irradiance; apply Lambertian BRDF factor and intensity multiplier
        irradiance = SampleDDGI(worldPos, N) * INV_PI * u_DDGIIntensity;
    } else if (u_EnableIBL != 0 && textureSize(u_IrradianceMap, 0).x > 1) {
        irradiance = texture(u_IrradianceMap, N).rgb;
    }


    vec3 diffuseIBL = irradiance * albedo * (1.0 - metallic);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = vec3(0.0);
    
    if (useRCSpecular) {
        prefilteredColor = rcSpecular * u_RCIntensity;
    } else if (u_EnableIBL != 0 && textureSize(u_PrefilterMap, 0).x > 1) {
        prefilteredColor = textureLod(u_PrefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
    }
    
    vec2 brdf = vec2(0.5, 0.0);
    if (textureSize(u_BrdfLut, 0).x > 1) {
        brdf = texture(u_BrdfLut, vec2(max(dot(N, V), 0.0), roughness)).rg;
    }
    
    vec3 specularIBL = prefilteredColor * (F0 * brdf.x + brdf.y);

    float rtao = (u_EnableRTAO != 0) ? texture(u_AOTexture, v_TexCoord).r : 1.0;
    vec3 ambient = (diffuseIBL + specularIBL) * ao * rtao;

    vec3 color = ambient + Lo;

    if (isEmissive > 0.5) {
        color += albedo * emissiveStrength;
    }

    if (u_RCDebugCategory == 1 && u_RCDebugMode == 0) {
        color = vec3(1.0, 1.0, 0.0);
    } else if (u_RCDebugCategory == 2) {
        color = texelFetch(u_RadianceAtlas, ivec2(gl_FragCoord.xy), 0).rgb;
    } else if (u_RCDebugCategory == 3) {
        if (u_RCDebugMode == 0) {
            color = texelFetch(u_RadianceAtlas, ivec2(gl_FragCoord.xy), 0).rgb;
        } else {
            // Already computed in irradiance, just show it
            color = irradiance;
        }
    } else if (u_RCDebugCategory == 4) {
        if (u_RCDebugMode == 0) {
            color = diffuseIBL; // Diffuse Only
        } else if (u_RCDebugMode == 1) {
            color = specularIBL; // Specular Only
        } else if (u_RCDebugMode == 2) {
            color = diffuseIBL + specularIBL; // Combined Indirect
        }
    }

    o_Color = vec4(color, 1.0);
}
