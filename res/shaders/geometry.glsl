#type vertex
#version 460 core
#extension GL_ARB_bindless_texture : require

struct InstanceData {
    mat4 Transform;
    uvec4 meshletInfo;
    vec4 color;
    vec4 pbr;
    uvec2 albedoMapHandle;
    uvec2 normalMapHandle;
    uvec2 pbrMapHandle;
    uvec2 padding;
};

layout(std430, binding = 4) readonly buffer GlobalVertexBuffer {
    float vertices[];
};

layout(std430, binding = 5) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

uniform mat4 u_ViewProjection;
uniform mat4 u_PrevViewProjection;

out vec3 v_WorldPos;
out vec3 v_WorldNormal;
out vec2 v_TexCoord;
out vec4 v_CurrentNDC;
out vec4 v_PrevNDC;
flat out vec4 v_Color;
flat out vec4 v_PBR;
flat out uvec2 v_AlbedoHandle;
flat out uvec2 v_NormalHandle;
flat out uvec2 v_PBRHandle;

void main() {
    uint baseIndex = gl_VertexID * 8;
    vec3 vPos = vec3(vertices[baseIndex], vertices[baseIndex+1], vertices[baseIndex+2]);
    vec3 vNorm = vec3(vertices[baseIndex+3], vertices[baseIndex+4], vertices[baseIndex+5]);
    vec2 vTex = vec2(vertices[baseIndex+6], vertices[baseIndex+7]);
    
    mat4 model = instances[gl_BaseInstance].Transform;
    
    vec4 worldPos = model * vec4(vPos, 1.0);
    v_WorldPos = worldPos.xyz;
    v_WorldNormal = mat3(transpose(inverse(model))) * vNorm;
    v_TexCoord = vTex;
    
    v_CurrentNDC = u_ViewProjection * worldPos;
    v_PrevNDC = u_PrevViewProjection * worldPos; 
    
    v_Color = instances[gl_BaseInstance].color;
    v_PBR = instances[gl_BaseInstance].pbr;
    v_AlbedoHandle = instances[gl_BaseInstance].albedoMapHandle;
    v_NormalHandle = instances[gl_BaseInstance].normalMapHandle;
    v_PBRHandle = instances[gl_BaseInstance].pbrMapHandle;
    
    gl_Position = v_CurrentNDC;
}

#type fragment
#version 460 core
#extension GL_ARB_bindless_texture : require

layout(location = 0) out vec4 gAlbedo;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gPBR;
layout(location = 3) out vec2 gVelocity;

in vec3 v_WorldPos;
in vec3 v_WorldNormal;
in vec2 v_TexCoord;
in vec4 v_CurrentNDC;
in vec4 v_PrevNDC;
flat in vec4 v_Color;
flat in vec4 v_PBR;
flat in uvec2 v_AlbedoHandle;
flat in uvec2 v_NormalHandle;
flat in uvec2 v_PBRHandle;

uniform float u_EmissiveStrength;

vec2 OctEncode(vec3 n) {
    n /= abs(n.x) + abs(n.y) + abs(n.z);
    if (n.z < 0.0) n.xy = (1.0 - abs(n.yx)) * sign(n.xy);
    return n.xy * 0.5 + 0.5;
}

vec3 getNormalFromMap(vec3 worldPos, vec3 normal, vec2 uv) {
    if (v_NormalHandle == uvec2(0)) return normalize(normal);
    
    sampler2D normalTex = sampler2D(v_NormalHandle);
    vec3 tangentNormal = texture(normalTex, uv).rgb * 2.0 - 1.0;
    
    vec3 Q1  = dFdx(worldPos);
    vec3 Q2  = dFdy(worldPos);
    vec2 st1 = dFdx(uv);
    vec2 st2 = dFdy(uv);
    
    vec3 N  = normalize(normal);
    
    float denom = (st1.s * st2.t - st2.s * st1.t);
    if (abs(denom) < 0.0001) {
        return N;
    }
    
    vec3 T  = (Q1 * st2.t - Q2 * st1.t) / denom;
    T = normalize(T - dot(T, N) * N);
    vec3 B  = cross(N, T);
    mat3 TBN = mat3(T, B, N);
    
    return normalize(TBN * tangentNormal);
}

void main() {
    vec3 albedo = v_Color.rgb;
    if (v_AlbedoHandle != uvec2(0)) {
        sampler2D albedoTex = sampler2D(v_AlbedoHandle);
        vec4 texColor = texture(albedoTex, v_TexCoord);
        albedo *= texColor.rgb;
    }
    
    float isEmissive = (u_EmissiveStrength > 0.5) ? 1.0 : 0.0;
    gAlbedo = vec4(albedo, isEmissive);
    
    vec3 finalNormal = getNormalFromMap(v_WorldPos, v_WorldNormal, v_TexCoord);
    vec2 octNormal = OctEncode(finalNormal);
    gNormal = vec4(octNormal, 0.0, 0.0);
    
    float metallic = v_PBR.r;
    float roughness = v_PBR.g;
    float ao = v_PBR.b;
    if (v_PBRHandle != uvec2(0)) {
        sampler2D pbrTex = sampler2D(v_PBRHandle);
        vec3 pbrTexColor = texture(pbrTex, v_TexCoord).rgb;
        roughness *= pbrTexColor.g;
        metallic *= pbrTexColor.b;
    }
    gPBR = vec4(metallic, roughness, ao, u_EmissiveStrength);
    
    vec2 currentPos = (v_CurrentNDC.xy / v_CurrentNDC.w);
    vec2 prevPos = (v_PrevNDC.xy / v_PrevNDC.w);
    vec2 velocity = (currentPos - prevPos) * 0.5;
    gVelocity = velocity;
}
