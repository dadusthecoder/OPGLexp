#ifndef BRDF_GLSL
#define BRDF_GLSL

#include "sampling.glsl"

// Disney Diffuse model
vec3 DisneyDiffuse(float NdotV, float NdotL, float LdotH, float roughness, vec3 albedo) {
    float Fd90 = 0.5 + 2.0 * LdotH * LdotH * roughness;
    // Schlick's approximation for Fresnel reflection
    float lightScatter = 1.0 + (Fd90 - 1.0) * pow(clamp(1.0 - NdotL, 0.0, 1.0), 5.0);
    float viewScatter  = 1.0 + (Fd90 - 1.0) * pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);
    return albedo * INV_PI * lightScatter * viewScatter;
}

// Disney's GTR2 (GGX) Normal Distribution Function
float D_GTR2(float NdotH, float roughness) {
    float alpha = roughness * roughness;
    float a2 = alpha * alpha;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom + EPSILON);
}

// Disney's Smith GGX Geometry term (uses remapped roughness for direct lights)
float G_SmithGGX_Disney(float NdotV, float NdotL, float roughness) {
    // Remap roughness for analytical lights to reduce hot-spots (Burley 2012)
    float r = (roughness + 1.0) * 0.5;
    float k = (r * r) * 0.5;
    float GV = NdotV / (NdotV * (1.0 - k) + k);
    float GL = NdotL / (NdotL * (1.0 - k) + k);
    return GV * GL;
}

// Fresnel-Schlick
vec3 F_Schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
vec3 F_SchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Full Disney Principled BRDF for Direct Light
vec3 DirectLight(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic, float roughness, vec3 lightColor) {
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL < EPSILON) return vec3(0.0);

    vec3 H = normalize(V + L);
    float NdotV = max(dot(N, V), EPSILON);
    float NdotH = max(dot(N, H), 0.0);
    float LdotH = max(dot(L, H), 0.0);

    // F0 assumes 0.04 for dielectrics (IOR ~1.5)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    
    // Fresnel term (Schlick)
    vec3 F = F_Schlick(LdotH, F0);

    // Energy conservation: diffuse energy is 1.0 - specular
    // Pure metals have no diffuse contribution
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

    // Disney Diffuse (accounts for roughness and retro-reflection)
    vec3 diffuse = kD * DisneyDiffuse(NdotV, NdotL, LdotH, roughness, albedo);

    // Disney Specular (GTR2 / modified GGX)
    float NDF = D_GTR2(NdotH, roughness);
    float G   = G_SmithGGX_Disney(NdotV, NdotL, roughness);
    vec3 specular = (NDF * G * F) / (4.0 * NdotV * NdotL + EPSILON);

    return (diffuse + specular) * lightColor * NdotL;
}

// Smith GGX Geometry term for IBL
float G_SmithGGX_IBL(float NdotV, float NdotL, float roughness) {
    // For IBL, k = alpha / 2 = roughness^2 / 2
    float k = (roughness * roughness) / 2.0;
    float GV = NdotV / (NdotV * (1.0 - k) + k);
    float GL = NdotL / (NdotL * (1.0 - k) + k);
    return GV * GL;
}

// Integrate BRDF LUT value (NdotV, roughness -> scale, bias)
vec2 IntegrateBRDF(float NdotV, float roughness, uint sampleCount) {
    vec3 V = vec3(sqrt(1.0 - NdotV*NdotV), 0.0, NdotV);
    float A = 0.0, B = 0.0;
    vec3 N = vec3(0.0, 0.0, 1.0);
    for (uint i = 0u; i < sampleCount; i++) {
        vec2 xi = Hammersley(i, sampleCount);
        vec3 H  = ImportanceSampleGGX(xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V,H) * H - V);
        float NdotL = clamp(L.z, 0.0, 1.0);
        float NdotH = clamp(H.z, 0.0, 1.0);
        float VdotH = clamp(dot(V,H), 0.0, 1.0);
        if (NdotL > 0.0) {
            float G    = G_SmithGGX_IBL(NdotV, NdotL, roughness);
            float GVis = (G * VdotH) / (NdotH * NdotV + EPSILON);
            float Fc   = pow(1.0 - VdotH, 5.0);
            A += (1.0 - Fc) * GVis;
            B += Fc * GVis;
        }
    }
    return vec2(A, B) / float(sampleCount);
}

#endif
