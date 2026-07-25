#shader Vertex
#version 460 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 textcoord;
layout(location = 3) in vec4 tangent;

layout(location = 0) out vec3 WorldPos;
layout(location = 1) out vec2 Textcoord;
layout(location = 2) out mat3 TBN;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main() {
    vec3 T = normalize(mat3(u_Model) * tangent.xyz);
    vec3 N = normalize(mat3(u_Model) * normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T) * tangent.w;

    TBN = mat3(T, B, N);

    WorldPos = vec3(u_Model * vec4(pos, 1.0));
    Textcoord = textcoord;

    gl_Position = u_Projection * u_View * vec4(WorldPos, 1.0);
}

#shader Fragment
#version 460 core
#extension GL_ARB_bindless_texture : require

struct Material {
    vec4 baseColor;
    vec4 emmisiveColor;

    float roughness;
    float metallic;
    float emmisiveStrength;

    sampler2D  normalMap;
    sampler2D  diffuseMap;
    sampler2D  emmisiveMap;
};

struct Light {
    vec4 position; // xyz=pos, w=type (0=Point, 1=Dir, 2=Spot)
    vec4 color;    // rgb=col, a=intensity
    vec4 direction;// xyz=dir, w=radius
    vec4 params;   // x=innerCos, y=outerCos
};

layout(location = 0) in vec3 WorldPos;
layout(location = 1) in vec2 Textcoord;
layout(location = 2) in mat3 TBN;

layout(location = 0) out vec4 out_color;

layout(std430, binding = 0) buffer Materials {
    Material materials[];
};

layout(std430, binding = 1) readonly buffer LightBuffer {
    Light lights[];
};

layout(std430, binding = 2) readonly buffer VisibleLightIndicesBuffer {
    int visibleLightIndices[];
};

uniform int u_MaterialIndex;
uniform int u_DebugMode;
uniform vec3 u_CameraPos;
uniform vec2 u_ScreenSize;
uniform bool u_VisualizeTiles;
uniform bool u_SSAOEnabled;

layout(binding = 0) uniform samplerCube u_IrradianceMap;
layout(binding = 1) uniform samplerCube u_PrefilterMap;
layout(binding = 2) uniform sampler2D u_BrdfLUT;
layout(binding = 5) uniform sampler2D u_AOTexture;

layout(binding = 3) uniform sampler2DArrayShadow u_ShadowMap;
uniform int u_CascadeCount;
uniform float u_CascadePlaneDistances[4];
uniform mat4 u_LightSpaceMatrices[4];
uniform mat4 u_View;
uniform bool u_HasIBL;  // Whether IBL maps are loaded

float SampleShadowCascade(int layer, vec3 fragPosWorldSpace, vec3 normal, vec3 lightDir, float depthValue) {
    if (layer > u_CascadeCount) return 0.0;
    
    vec4 fragPosLightSpace = u_LightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    // Check if out of bounds of the orthographic projection
    if(projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;
        
    float currentDepth = projCoords.z;
    float bias = max(0.002 * (1.0 - dot(normal, lightDir)), 0.0002);
    float planeDist = layer == u_CascadeCount ? 100.0 : u_CascadePlaneDistances[layer];
    bias *= 1.0 / (planeDist * 0.5);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(u_ShadowMap, 0).xy);
    
    // 3x3 PCF using hardware shadow sampling (gives effective 4x4 PCF)
    // Hardware PCF with GL_LEQUAL returns 1.0 when LIT, 0.0 when SHADOW.
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            shadow += texture(u_ShadowMap, vec4(projCoords.xy + vec2(x, y) * texelSize, layer, currentDepth - bias)); 
        }    
    }
    float visibility = shadow / 9.0;
    
    // We return 'shadow' amount, so 1.0 means fully shadowed, 0.0 means fully lit
    return 1.0 - visibility;
}

float ShadowCalculation(vec3 fragPosWorldSpace, vec3 normal, vec3 lightDir) {
    vec4 fragPosViewSpace = u_View * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);
    
    int layer = -1;
    for (int i = 0; i < u_CascadeCount; ++i) {
        if (depthValue < u_CascadePlaneDistances[i]) {
            layer = i;
            break;
        }
    }
    if (layer == -1) {
        layer = u_CascadeCount; 
    }
    
    float shadow = SampleShadowCascade(layer, fragPosWorldSpace, normal, lightDir, depthValue);
    
    // Smooth cascade transition blending
    if (layer < u_CascadeCount) {
        float fadeDistance = 3.0; // blend over 3 units
        float planeDist = u_CascadePlaneDistances[layer];
        if (depthValue > planeDist - fadeDistance) {
            float nextShadow = SampleShadowCascade(layer + 1, fragPosWorldSpace, normal, lightDir, depthValue);
            float blendFactor = (depthValue - (planeDist - fadeDistance)) / fadeDistance;
            shadow = mix(shadow, nextShadow, clamp(blendFactor, 0.0, 1.0));
        }
    }
    
    return shadow;
}

float GGX(float NdotH, float roughness) {
    float roughness2 = roughness * roughness;
    float denom = ((NdotH * NdotH * (roughness2 - 1.0)) + 1.0);
    return roughness2 / (3.14159265 * denom * denom);
}

float Fschlick(float F0, float HdotV) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - HdotV, 0.0, 1.0), 5.0);
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

void main() {
    float roughness = clamp(materials[u_MaterialIndex].roughness, 0.05, 1.0);
    float metallic  = materials[u_MaterialIndex].metallic;

    vec4 albedo   = texture(materials[u_MaterialIndex].diffuseMap, Textcoord) * materials[u_MaterialIndex].baseColor;
    if (albedo.a < 0.5) {
        discard;
    }
    vec4 emmisive = texture(materials[u_MaterialIndex].emmisiveMap, Textcoord) * materials[u_MaterialIndex].emmisiveColor;

    vec3 normalMap = texture(materials[u_MaterialIndex].normalMap, Textcoord).rgb;
    vec3 N = normalize(TBN * (normalMap * 2.0 - 1.0));
    vec3 V = normalize(u_CameraPos - WorldPos);
    float NdotV = clamp(dot(N, V), 0.0, 1.0);

    vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic); 

    // Tile look up for lights
    ivec2 tileID = ivec2(gl_FragCoord.xy) / 16;
    int numTilesX = (int(u_ScreenSize.x) + 15) / 16;
    uint tileIndex = tileID.y * numTilesX + tileID.x;
    uint offset = tileIndex * 257;
    int lightCount = visibleLightIndices[offset];

    vec3 finalColor = vec3(0.0);
    
    // IBL Ambient Lighting
    vec3 F = FschlickVec3(F0, NdotV); // Note: Should actually be FschlickRoughness but Fschlick is okay for simple version
    // A better F for IBL:
    vec3 F_IBL = F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);
    vec3 kS = F_IBL;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    
    vec3 ambient = vec3(0.0);
    if (u_HasIBL) {
        vec3 irradiance = texture(u_IrradianceMap, N).rgb;
        vec3 diffuse    = irradiance * albedo.rgb;
        
        vec3 R = reflect(-V, N);
        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(u_PrefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;    
        vec2 brdf  = texture(u_BrdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 specular = prefilteredColor * (F_IBL * brdf.x + brdf.y);
        ambient = (kD * diffuse + specular);
    } else {
        // Fallback constant ambient when no IBL is loaded
        ambient = albedo.rgb * 0.03;
    }

    // Apply SSAO
    if (u_SSAOEnabled) {
        vec2 screenUV = gl_FragCoord.xy / u_ScreenSize;
        float ao = texture(u_AOTexture, screenUV).r;
        ambient *= ao;
    }

    finalColor += ambient;

    for (int i = 0; i < lightCount; i++) {
        int lightIdx = visibleLightIndices[offset + 1 + i];
        Light light = lights[lightIdx];

        int type = int(light.position.w);
        vec3 L;
        float attenuation = 1.0;
        float shadow = 0.0;
        
        if (type == 1) { // Directional
            L = normalize(-light.direction.xyz);
            shadow = ShadowCalculation(WorldPos, N, L);
        } else { // Point or Spot
            L = light.position.xyz - WorldPos;
            float dist = length(L);
            L = normalize(L);
            float radius = light.direction.w;
            attenuation = clamp(1.0 - (dist * dist) / (radius * radius), 0.0, 1.0);
            attenuation *= attenuation;

            if (type == 2) { // Spot
                float theta = dot(L, normalize(-light.direction.xyz));
                float innerCutoff = light.params.x;
                float outerCutoff = light.params.y;
                float epsilon = innerCutoff - outerCutoff;
                float intensity = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);
                attenuation *= intensity;
            }
        }

        vec3 H = normalize(L + V);
        vec3 radiance = light.color.rgb * light.color.a * attenuation;

        float NdotL = clamp(dot(N, L), 0.0, 1.0);
        float NdotH = clamp(dot(N, H), 0.0, 1.0);
        float LdotV = clamp(dot(L, V), 0.0, 1.0);
        float LdotH = clamp(dot(L, H), 0.0, 1.0);

        float D = GGX(NdotH, roughness);
        float G = SmithGGX(NdotV, NdotL, roughness);
        vec3 F = FschlickVec3(F0, LdotH);

        vec3 specularTerm = D * G * F; // G already contains 1/(4*NdotV*NdotL)
        float diff = DisneyDiffuse(NdotL, LdotV, LdotH, roughness);
        vec3 diffuseTerm = albedo.rgb * diff;

        vec3 kd = (vec3(1.0) - F) * (1.0 - metallic);
        finalColor += (kd * diffuseTerm + specularTerm) * radiance * NdotL * (1.0 - shadow);
    }

    finalColor += emmisive.rgb * materials[u_MaterialIndex].emmisiveStrength;

    // Linear to Gamma
    finalColor = finalColor / (finalColor + vec3(1.0));
    finalColor = pow(finalColor, vec3(1.0/2.2));

    if (u_VisualizeTiles) {
        // Tile outline
        vec2 tileUV = mod(gl_FragCoord.xy, 16.0);
        if (tileUV.x < 1.0 || tileUV.y < 1.0) {
            finalColor += vec3(0.1); 
        }
        
        // Heatmap based on light count (0 to ~30 lights)
        float heat = float(lightCount) / 30.0;
        vec3 heatColor = mix(vec3(0, 0, 1), vec3(1, 0, 0), heat); 
        // Purple/White for extremely hot
        if (heat > 1.0) heatColor = mix(vec3(1, 0, 0), vec3(1, 1, 1), heat - 1.0);
        
        finalColor = mix(finalColor, heatColor, 0.4);
    }

    switch(u_DebugMode){
        case 1:  out_color = vec4(albedo.rgb, 1.0); break;      // Albedo
        case 2:  out_color = vec4(normalMap, 1.0); break;       // Normal Map
        case 3:  out_color = vec4(emmisive.rgb, 1.0); break;    // Emissive
        case 4:  out_color = vec4(N * 0.5 + 0.5, 1.0); break;  // World Normal
        default: out_color = vec4(finalColor, 1.0); break;      // 0 = Final PBR
    }
}
