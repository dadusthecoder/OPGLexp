// =============================================================================
// DeferredLighting.shader
// Evaluates PBR lighting reading from the G-Buffer
// =============================================================================

#shader Vertex
#version 460 core

// Fullscreen triangle trick
out vec2 v_TexCoord;

void main() {
    vec2 position = vec2(
        float((gl_VertexID << 1) & 2) * 2.0 - 1.0,
        float(gl_VertexID & 2) * 2.0 - 1.0
    );
    v_TexCoord = position * 0.5 + 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
}

#shader Fragment
#version 460 core
#extension GL_ARB_bindless_texture : require

in vec2 v_TexCoord;
layout(location = 0) out vec4 FragColor;

// G-Buffer inputs
layout(binding = 0) uniform sampler2D u_gAlbedoMetallic;
layout(binding = 1) uniform sampler2D u_gNormalRoughness;
layout(binding = 2) uniform sampler2D u_gEmissive;
layout(binding = 3) uniform sampler2D u_gDepth;

struct Light {
    vec4 position; // xyz=pos, w=type (0=Point, 1=Dir, 2=Spot)
    vec4 color;    // rgb=col, a=intensity
    vec4 direction;// xyz=dir, w=radius
    vec4 params;   // x=innerCos, y=outerCos
};

layout(std430, binding = 1) readonly buffer LightBuffer {
    Light lights[];
};

layout(std430, binding = 2) readonly buffer VisibleLightIndicesBuffer {
    int visibleLightIndices[];
};

uniform vec3 u_CameraPos;
uniform vec2 u_ScreenSize;
uniform mat4 u_InverseView;
uniform mat4 u_InverseProjection;
uniform mat4 u_View;

layout(binding = 4) uniform samplerCube u_IrradianceMap;
layout(binding = 5) uniform samplerCube u_PrefilterMap;
layout(binding = 6) uniform sampler2D u_BrdfLUT;
layout(binding = 7) uniform sampler2D u_AOTexture;
layout(binding = 8) uniform sampler2DArrayShadow u_ShadowMap; // for PCF comparison

uniform int u_CascadeCount;
uniform float u_CascadePlaneDistances[4];
uniform mat4 u_LightSpaceMatrices[4];
uniform bool u_HasIBL;
uniform bool u_SSAOEnabled;
uniform int u_DebugMode;

// ============================================================
// Shadow debug mode — visualizes individual pipeline stages.
//
//  0 = normal rendering (no override)
//  1 = WorldPos (should be camera-INDEPENDENT — fixed in world)
//  2 = ViewPos  (should CHANGE with camera rotation)
//  3 = light-space clip pos before w-divide (should be FIXED)
//  4 = projCoords after w-divide + [0,1] remap (should be FIXED)
//  5 = shadow UV (projCoords.xy — should be FIXED)
//  6 = shadow map stored depth at UV (should be FIXED)
//  7 = current fragment depth in light space (projCoords.z — FIXED)
//  8 = cascade index (colour-coded — should be FIXED)
//  9 = raw shadow comparison (0=lit, 1=shadowed) — should be FIXED
// 10 = final shadow factor after PCF blend — should be FIXED
//
// If ANY mode 1,3-10 changes when you ONLY rotate the camera,
// the bug is in the stage that produces that value.
// ============================================================
uniform int u_ShadowDebugMode; // 0 = off, 1-10 as above

// ============================================================
// Reconstruct world space position from depth
// ============================================================
vec3 WorldPosFromDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clipSpacePosition = vec4(v_TexCoord * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = u_InverseProjection * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;
    vec4 worldSpacePosition = u_InverseView * viewSpacePosition;
    return worldSpacePosition.xyz;
}

// ============================================================
// Shadow helpers — fully instrumented
// ============================================================

// Returns shadow factor [0=lit, 1=fully-shadowed] for one cascade.
// All intermediate values are also returned via out-parameters for debug.
float SampleShadowCascadeDebug(
    int layer,
    vec3 fragPosWorldSpace,
    vec3 normal,
    vec3 lightDir,
    float viewDepth,
    // debug out-params
    out vec4 dbg_lightClip,        // raw light-space clip coords (before /w)
    out vec3 dbg_projCoords,       // NDC->UV-remapped coords (after /w)
    out vec2 dbg_shadowUV,         // xy lookup into shadow map
    out float dbg_storedDepth,     // depth value read from shadow map
    out float dbg_currentDepth,    // current frag depth in light space
    out float dbg_rawComparison    // 0=lit, 1=shadowed before PCF
) {
    // --- defaults in case we early-out ---
    dbg_lightClip      = vec4(0.0);
    dbg_projCoords     = vec3(0.0);
    dbg_shadowUV       = vec2(0.0);
    dbg_storedDepth    = 0.0;
    dbg_currentDepth   = 0.0;
    dbg_rawComparison  = 0.0;

    if (layer >= u_CascadeCount) return 0.0;

    // Step A: transform fragment world-pos into light clip space
    vec4 fragPosLightSpace = u_LightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);
    dbg_lightClip = fragPosLightSpace;

    // Step B: perspective divide + remap to [0,1]
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    dbg_projCoords = projCoords;
    dbg_shadowUV   = projCoords.xy;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    // Step C: depth values
    float currentDepth = projCoords.z;
    dbg_currentDepth = currentDepth;

    // Cannot read raw depth from a COMPARE_REF_TO_TEXTURE sampler due to undefined behavior.
    dbg_storedDepth = 0.0;

    // Step D: bias
    float bias = max(0.002 * (1.0 - dot(normal, lightDir)), 0.0002);
    float planeDist = (layer == u_CascadeCount) ? 100.0 : u_CascadePlaneDistances[layer];
    bias *= 1.0 / (planeDist * 0.5);

    // Step E: raw comparison (centre texel only, no PCF)
    dbg_rawComparison = (currentDepth - bias > dbg_storedDepth) ? 1.0 : 0.0;

    // Step F: 3x3 PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(u_ShadowMap, 0).xy);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            shadow += texture(u_ShadowMap,
                vec4(projCoords.xy + vec2(x, y) * texelSize, layer, currentDepth - bias));
        }
    }
    return 1.0 - (shadow / 9.0);
}

// Top-level shadow with cascade selection — returns final factor,
// populates all debug vars for the SELECTED cascade only.
float ShadowCalculationDebug(
    vec3 fragPosWorldSpace,
    vec3 normal,
    vec3 lightDir,
    out int   dbg_cascade,
    out vec4  dbg_lightClip,
    out vec3  dbg_projCoords,
    out vec2  dbg_shadowUV,
    out float dbg_storedDepth,
    out float dbg_currentDepth,
    out float dbg_rawComparison
) {
    // Cascade selection uses VIEW-SPACE depth — this is intentionally camera-dependent.
    vec4 fragPosViewSpace = u_View * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < u_CascadeCount; ++i) {
        if (depthValue < u_CascadePlaneDistances[i]) {
            layer = i;
            break;
        }
    }
    if (layer == -1) layer = u_CascadeCount - 1;
    dbg_cascade = layer;

    float shadow = SampleShadowCascadeDebug(
        layer, fragPosWorldSpace, normal, lightDir, depthValue,
        dbg_lightClip, dbg_projCoords, dbg_shadowUV,
        dbg_storedDepth, dbg_currentDepth, dbg_rawComparison
    );

    // Cascade blend
    if (layer < u_CascadeCount - 1) {
        float fadeDistance = 3.0;
        float planeDist = u_CascadePlaneDistances[layer];
        if (depthValue > planeDist - fadeDistance) {
            vec4 d2; vec3 p2; vec2 uv2; float sd2, cd2, rc2;
            float nextShadow = SampleShadowCascadeDebug(
                layer + 1, fragPosWorldSpace, normal, lightDir, depthValue,
                d2, p2, uv2, sd2, cd2, rc2
            );
            float blendFactor = (depthValue - (planeDist - fadeDistance)) / fadeDistance;
            shadow = mix(shadow, nextShadow, clamp(blendFactor, 0.0, 1.0));
        }
    }

    return shadow;
}

// ============================================================
// PBR functions
// ============================================================
float GGX(float NdotH, float roughness) {
    float roughness2 = roughness * roughness;
    float denom = ((NdotH * NdotH * (roughness2 - 1.0)) + 1.0);
    return roughness2 / (3.14159265 * denom * denom);
}
vec3 FschlickVec3(vec3 F0, float HdotV) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - HdotV, 0.0, 1.0), 5.0);
}
float DisneyDiffuse(float NdotL, float LdotV, float LdotH, float roughness) {
    float F90 = 0.5 + 2.0 * (roughness * LdotH * LdotH);
    float F0 = 1.0;
    float diffuse = mix(F0, F90, NdotL) * mix(F0, F90, LdotV);
    return diffuse / 3.14159265;
}
float SmithGGX(float NdotV, float NdotL, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float GGXL = NdotV * sqrt(a2 + NdotL * (NdotL - a2 * NdotL));
    float GGXV = NdotL * sqrt(a2 + NdotV * (NdotV - a2 * NdotV));
    if (GGXV + GGXL == 0.0) return 0.0;
    return 0.5 / (GGXV + GGXL);
}

// ============================================================
// Main
// ============================================================
void main() {
    float depth = texture(u_gDepth, v_TexCoord).r;
    if (depth >= 1.0) {
        discard;
    }

    // --- Reconstruct positions ---
    vec3 WorldPos   = WorldPosFromDepth(depth);
    vec4 ViewPos4   = u_View * vec4(WorldPos, 1.0);
    vec3 ViewPos    = ViewPos4.xyz;          // debug mode 2

    // --- G-Buffer reads ---
    vec4 albedoMetallic = texture(u_gAlbedoMetallic, v_TexCoord);
    vec3 albedo    = albedoMetallic.rgb;
    float metallic = albedoMetallic.a;

    vec4 normalRoughness = texture(u_gNormalRoughness, v_TexCoord);
    vec3 N         = normalize(normalRoughness.rgb);
    float roughness = clamp(normalRoughness.a, 0.05, 1.0);

    vec3 emissive = texture(u_gEmissive, v_TexCoord).rgb;

    vec3 V      = normalize(u_CameraPos - WorldPos);
    float NdotV = clamp(dot(N, V), 0.0, 1.0);
    vec3 F0     = mix(vec3(0.04), albedo, metallic);

    // --- Find directional light for shadow debug ---
    ivec2 tileID  = ivec2(gl_FragCoord.xy) / 16;
    int numTilesX = (int(u_ScreenSize.x) + 15) / 16;
    uint tileIndex = uint(tileID.y * numTilesX + tileID.x);
    uint offset    = tileIndex * 257u;
    int lightCount = visibleLightIndices[offset];

    vec3 dirL = vec3(0.0, -1.0, 0.0); // fallback light dir
    for (int i = 0; i < lightCount; i++) {
        int lightIdx = visibleLightIndices[offset + 1u + uint(i)];
        if (int(lights[lightIdx].position.w) == 1) { // directional
            dirL = normalize(-lights[lightIdx].direction.xyz);
            break;
        }
    }

    // --- Full instrumented shadow calc ---
    int   dbg_cascade;
    vec4  dbg_lightClip;
    vec3  dbg_projCoords;
    vec2  dbg_shadowUV;
    float dbg_storedDepth;
    float dbg_currentDepth;
    float dbg_rawComparison;

    float shadow = ShadowCalculationDebug(
        WorldPos, N, dirL,
        dbg_cascade,
        dbg_lightClip,
        dbg_projCoords,
        dbg_shadowUV,
        dbg_storedDepth,
        dbg_currentDepth,
        dbg_rawComparison
    );

    // ============================================================
    // SHADOW DEBUG VISUALIZATIONS
    // ============================================================
    if (u_ShadowDebugMode != 0) {
        vec3 debugColor = vec3(0.0);

        if (u_ShadowDebugMode == 1) {
            // World position — MUST be camera-independent.
            // Encode as fract() so large coords wrap into [0,1] visible range.
            // A pixel on a static wall should show a CONSTANT colour.
            debugColor = fract(WorldPos * 0.1);

        } else if (u_ShadowDebugMode == 2) {
            // View-space position — WILL change with camera rotation (that is correct).
            // Used to confirm the view matrix itself is not the root issue.
            debugColor = fract(abs(ViewPos) * 0.1);

        } else if (u_ShadowDebugMode == 3) {
            // Light-space clip pos BEFORE perspective divide — MUST be camera-independent.
            // Show xyz remapped to [0,1]. If this shifts when camera rotates, WorldPos is wrong.
            debugColor = dbg_lightClip.xyz * 0.5 + 0.5;

        } else if (u_ShadowDebugMode == 4) {
            // projCoords AFTER w-divide + [0,1] remap — MUST be camera-independent.
            // This should look like a static UV-mapped picture of the shadow frustum.
            debugColor = fract(dbg_projCoords * 10.0);

        } else if (u_ShadowDebugMode == 5) {
            // Shadow UV (projCoords.xy) — MUST be camera-independent.
            // Shows which texel in the shadow map is being sampled.
            debugColor = vec3(fract(dbg_shadowUV * 10.0), 0.0);

        } else if (u_ShadowDebugMode == 6) {
            // Stored depth in shadow map at that UV — MUST be camera-independent.
            // This is a property of the shadow map geometry, not the camera.
            debugColor = vec3(dbg_storedDepth);

        } else if (u_ShadowDebugMode == 7) {
            // Current fragment depth in light space (projCoords.z) — MUST be camera-independent.
            // This is determined by the fragment's world position and the light, not the camera.
            debugColor = vec3(dbg_currentDepth);

        } else if (u_ShadowDebugMode == 8) {
            // Cascade index colour-coded — SHOULD be mostly camera-independent.
            // Cascade SELECTION uses view-space depth, so near cascade boundaries the
            // selected cascade index can legitimately flip when camera moves.
            // But pixels far from boundaries should show a stable colour.
            vec3 cascadeColors[4] = vec3[4](
                vec3(1.0, 0.0, 0.0),  // cascade 0 = red   (closest)
                vec3(0.0, 1.0, 0.0),  // cascade 1 = green
                vec3(0.0, 0.0, 1.0),  // cascade 2 = blue
                vec3(1.0, 1.0, 0.0)   // cascade 3 = yellow (farthest)
            );
            debugColor = (dbg_cascade >= 0 && dbg_cascade < 4)
                ? cascadeColors[dbg_cascade]
                : vec3(1.0, 0.0, 1.0); // magenta = invalid
            // Dim so it's more readable over white geometry
            debugColor *= 0.8;

        } else if (u_ShadowDebugMode == 9) {
            // Raw shadow comparison (centre texel, no PCF) — MUST be camera-independent.
            // 0 = fragment is LIT  (white)
            // 1 = fragment is IN SHADOW (black)
            debugColor = vec3(1.0 - dbg_rawComparison);

        } else if (u_ShadowDebugMode == 10) {
            // Final shadow factor after PCF blend — MUST be camera-independent.
            // 0=lit (white), 1=shadowed (black)
            debugColor = vec3(1.0 - shadow);
        }

        FragColor = vec4(debugColor, 1.0);
        return;
    }

    // ============================================================
    // Normal PBR lighting path
    // ============================================================
    vec3 finalColor = vec3(0.0);

    vec3 F_IBL = F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);
    vec3 kS    = F_IBL;
    vec3 kD    = (1.0 - kS) * (1.0 - metallic);

    vec3 ambient = vec3(0.0);
    if (u_HasIBL) {
        vec3 irradiance      = texture(u_IrradianceMap, N).rgb;
        vec3 diffuse         = irradiance * albedo;
        vec3 R               = reflect(-V, N);
        const float MAX_LOD  = 4.0;
        vec3 prefilteredColor = textureLod(u_PrefilterMap, R, roughness * MAX_LOD).rgb;
        vec2 brdf            = texture(u_BrdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 specular        = prefilteredColor * (F_IBL * brdf.x + brdf.y);
        ambient              = kD * diffuse + specular;
    } else {
        ambient = albedo * 0.03;
    }

    if (u_SSAOEnabled) {
        float ao = texture(u_AOTexture, v_TexCoord).r;
        ambient *= ao;
    }
    finalColor += ambient;

    for (int i = 0; i < lightCount; i++) {
        int lightIdx = visibleLightIndices[offset + 1u + uint(i)];
        Light light  = lights[lightIdx];

        int   type        = int(light.position.w);
        vec3  L;
        float attenuation = 1.0;
        float sh          = 0.0;

        if (type == 1) { // Directional
            L  = normalize(-light.direction.xyz);
            sh = shadow; // reuse already-computed shadow
        } else {
            L   = light.position.xyz - WorldPos;
            float dist = length(L);
            L   = normalize(L);
            float radius = light.direction.w;
            attenuation  = clamp(1.0 - (dist * dist) / (radius * radius), 0.0, 1.0);
            attenuation *= attenuation;

            if (type == 2) { // Spot
                float theta      = dot(L, normalize(-light.direction.xyz));
                float innerCutoff = light.params.x;
                float outerCutoff = light.params.y;
                float epsilon    = innerCutoff - outerCutoff;
                float intensity  = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);
                attenuation     *= intensity;
            }
        }

        vec3  H      = normalize(L + V);
        vec3  radiance = light.color.rgb * light.color.a * attenuation;

        float NdotL = clamp(dot(N, L), 0.0, 1.0);
        float NdotH = clamp(dot(N, H), 0.0, 1.0);
        float LdotV = clamp(dot(L, V), 0.0, 1.0);
        float LdotH = clamp(dot(L, H), 0.0, 1.0);

        float D    = GGX(NdotH, roughness);
        float G    = SmithGGX(NdotV, NdotL, roughness);
        vec3  F    = FschlickVec3(F0, LdotH);

        vec3 specularTerm = D * G * F;
        float diff        = DisneyDiffuse(NdotL, LdotV, LdotH, roughness);
        vec3 diffuseTerm  = albedo * diff;

        vec3 kd = (vec3(1.0) - F) * (1.0 - metallic);
        finalColor += (kd * diffuseTerm + specularTerm) * radiance * NdotL * (1.0 - sh);
    }

    finalColor += emissive;

    // PBR G-Buffer debug views
    if      (u_DebugMode == 1) FragColor = vec4(albedo, 1.0);
    else if (u_DebugMode == 2) FragColor = vec4(N * 0.5 + 0.5, 1.0);
    else if (u_DebugMode == 3) FragColor = vec4(emissive, 1.0);
    else                       FragColor = vec4(finalColor, 1.0);
}
