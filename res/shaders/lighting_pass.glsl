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

uniform sampler2D u_gAlbedoSpec;
uniform sampler2D u_gNormal;
uniform sampler2D u_gPBR;
uniform sampler2D u_gDepth;

struct Light {
    vec3 Position;
    vec3 Direction;
    vec3 Color;
    float Intensity;
    int Type; // 0 = Directional, 1 = Point
    float Radius;
};

uniform Light u_Lights[32];
uniform int u_LightCount;
uniform mat4 u_InverseViewProjection;
uniform vec3 u_CameraPosition;

vec3 WorldPosFromDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clipPos = vec4(v_TexCoord * 2.0 - 1.0, z, 1.0);
    vec4 worldPos = u_InverseViewProjection * clipPos;
    return worldPos.xyz / worldPos.w;
}

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Smooth distance attenuation that reaches zero at Radius
float SmoothAttenuation(float distance, float radius) {
    float d = distance / max(radius, 0.001);
    float d2 = d * d;
    float d4 = d2 * d2;
    float falloff = clamp(1.0 - d4, 0.0, 1.0);
    return (falloff * falloff) / (distance * distance + 1.0);
}

void main() {
    float depth = texture(u_gDepth, v_TexCoord).r;
    if (depth >= 1.0) {
        o_Color = vec4(0.05, 0.05, 0.08, 1.0);
        return;
    }

    vec3 WorldPos = WorldPosFromDepth(depth);
    vec3 albedo = texture(u_gAlbedoSpec, v_TexCoord).rgb;
    vec3 normal = normalize(texture(u_gNormal, v_TexCoord).rgb);
    vec3 pbr = texture(u_gPBR, v_TexCoord).rgb;
    
    float metallic = pbr.r;
    float roughness = max(pbr.g, 0.04); // Clamp to avoid division issues
    float ao = pbr.b;

    vec3 N = normal;
    vec3 V = normalize(u_CameraPosition - WorldPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);

    for (int i = 0; i < u_LightCount; ++i) {
        vec3 L;
        float attenuation = 1.0;
        
        if (u_Lights[i].Type == 0) {
            // Directional light
            L = normalize(-u_Lights[i].Direction);
        } else {
            // Point light with radius-based attenuation
            L = normalize(u_Lights[i].Position - WorldPos);
            float distance = length(u_Lights[i].Position - WorldPos);
            attenuation = SmoothAttenuation(distance, u_Lights[i].Radius);
        }

        vec3 H = normalize(V + L);
        vec3 radiance = u_Lights[i].Color * u_Lights[i].Intensity * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;
    
    o_Color = vec4(color, 1.0);
}
