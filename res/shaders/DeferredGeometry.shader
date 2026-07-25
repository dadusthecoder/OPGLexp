// =============================================================================
// DeferredGeometry.shader
// Renders the scene into the G-Buffer
// =============================================================================

#shader Vertex
#version 460 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 textcoord;
layout(location = 3) in vec4 tangent;

layout(location = 0) out vec3 WorldPos;
layout(location = 1) out vec2 Textcoord;
layout(location = 2) out mat3 TBN;
layout(location = 5) out vec4 ClipSpacePos;
layout(location = 6) out vec4 PrevClipSpacePos;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

// For velocity buffer
uniform mat4 u_PrevViewProj;

void main() {
    vec3 T = normalize(mat3(u_Model) * tangent.xyz);
    vec3 N = normalize(mat3(u_Model) * normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T) * tangent.w;

    TBN = mat3(T, B, N);

    WorldPos = vec3(u_Model * vec4(pos, 1.0));
    Textcoord = textcoord;

    ClipSpacePos = u_Projection * u_View * vec4(WorldPos, 1.0);
    // Assuming static models for now, prev model matrix is the same as current
    PrevClipSpacePos = u_PrevViewProj * vec4(WorldPos, 1.0);

    gl_Position = ClipSpacePos;
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

layout(std430, binding = 0) buffer Materials {
    Material materials[];
};

layout(location = 0) in vec3 WorldPos;
layout(location = 1) in vec2 Textcoord;
layout(location = 2) in mat3 TBN;
layout(location = 5) in vec4 ClipSpacePos;
layout(location = 6) in vec4 PrevClipSpacePos;

// G-Buffer Render Targets
layout(location = 0) out vec4 gAlbedoMetallic;
layout(location = 1) out vec4 gNormalRoughness;
layout(location = 2) out vec4 gEmissive;
layout(location = 3) out vec2 gVelocity;

uniform int u_MaterialIndex;
uniform int u_DebugMode;

void main() {
    float roughness = clamp(materials[u_MaterialIndex].roughness, 0.05, 1.0);
    float metallic  = materials[u_MaterialIndex].metallic;

    vec4 albedo = texture(materials[u_MaterialIndex].diffuseMap, Textcoord) * materials[u_MaterialIndex].baseColor;
    if (albedo.a < 0.5) {
        discard;
    }

    vec4 emissive = texture(materials[u_MaterialIndex].emmisiveMap, Textcoord) * materials[u_MaterialIndex].emmisiveColor;
    
    vec3 normalMap = texture(materials[u_MaterialIndex].normalMap, Textcoord).rgb;
    vec3 N = normalize(TBN * (normalMap * 2.0 - 1.0));

    // 1. Albedo + Metallic
    gAlbedoMetallic = vec4(albedo.rgb, metallic);

    // 2. Normal + Roughness (Normal mapped to [0,1] or we can just save it as is since it's FP16)
    // We are using GL_RGBA16F, so we can store normal in [-1, 1] range!
    gNormalRoughness = vec4(N, roughness);

    // 3. Emissive
    gEmissive = vec4(emissive.rgb * materials[u_MaterialIndex].emmisiveStrength, 1.0);

    // 4. Velocity
    // Calculate NDC space [-1, 1] for current and previous frame
    vec2 ndcPos = ClipSpacePos.xy / ClipSpacePos.w;
    vec2 prevNdcPos = PrevClipSpacePos.xy / PrevClipSpacePos.w;
    
    // Calculate velocity in UV space [0, 1] difference
    vec2 velocity = (ndcPos - prevNdcPos) * 0.5;
    gVelocity = velocity;
}
