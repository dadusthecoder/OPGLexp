#ifndef BRDF_GLSL
#define BRDF_GLSL

// GGX Normal Distribution Function
float D_GGX(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom + EPSILON);
}

// Smith GGX Geometry term
float G_SmithGGX(float NdotV, float NdotL, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
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

// Full Cook-Torrance specular BRDF (returns specular contribution)
vec3 CookTorranceSpecular(vec3 N, vec3 V, vec3 L, vec3 F0, float roughness) {
    vec3  H     = normalize(V + L);
    float NdotV = max(dot(N, V), EPSILON);
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    float NDF = D_GGX(NdotH, roughness);
    float G   = G_SmithGGX(NdotV, NdotL, roughness);
    vec3  F   = F_Schlick(HdotV, F0);

    return (NDF * G * F) / (4.0 * NdotV * NdotL + EPSILON);
}

// Full PBR direct light contribution
vec3 DirectLight(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic, float roughness, vec3 lightColor) {
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL < EPSILON) return vec3(0.0);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F  = F_Schlick(max(dot(normalize(V+L), V), 0.0), F0);
    vec3 kD = (1.0 - F) * (1.0 - metallic);

    vec3 diffuse  = kD * albedo * INV_PI;
    vec3 specular = CookTorranceSpecular(N, V, L, F0, roughness);

    return (diffuse + specular) * lightColor * NdotL;
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
            float G    = G_SmithGGX(NdotV, NdotL, roughness);
            float GVis = (G * VdotH) / (NdotH * NdotV + EPSILON);
            float Fc   = pow(1.0 - VdotH, 5.0);
            A += (1.0 - Fc) * GVis;
            B += Fc * GVis;
        }
    }
    return vec2(A, B) / float(sampleCount);
}

#endif
