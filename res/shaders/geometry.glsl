#type vertex
#version 460 core

struct InstanceData {
    mat4 Transform;
    uvec4 meshletInfo;
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
    
    gl_Position = v_CurrentNDC;
}

#type fragment
#version 460 core

layout(location = 0) out vec4 gAlbedo;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gPBR;
layout(location = 3) out vec2 gVelocity;

in vec3 v_WorldPos;
in vec3 v_WorldNormal;
in vec2 v_TexCoord;
in vec4 v_CurrentNDC;
in vec4 v_PrevNDC;

uniform vec3  u_Albedo;
uniform float u_Metallic;
uniform float u_Roughness;
uniform float u_EmissiveStrength;

layout(binding = 0) uniform sampler2D u_AlbedoTex;
layout(binding = 1) uniform sampler2D u_NormalTex;
layout(binding = 2) uniform sampler2D u_MetallicRoughnessTex;

uniform int u_HasAlbedoTex;
uniform int u_HasNormalTex;
uniform int u_HasMetallicRoughnessTex;

vec2 OctEncode(vec3 n) {
    n /= abs(n.x) + abs(n.y) + abs(n.z);
    if (n.z < 0.0) n.xy = (1.0 - abs(n.yx)) * sign(n.xy);
    return n.xy * 0.5 + 0.5;
}

vec3 getNormalFromMap(vec3 worldPos, vec3 normal, vec2 uv) {
    if (u_HasNormalTex == 0) return normalize(normal);
    
    vec3 tangentNormal = texture(u_NormalTex, uv).rgb * 2.0 - 1.0;
    
    vec3 Q1  = dFdx(worldPos);
    vec3 Q2  = dFdy(worldPos);
    vec2 st1 = dFdx(uv);
    vec2 st2 = dFdy(uv);
    
    vec3 N  = normalize(normal);
    
    float denom = (st1.s * st2.t - st2.s * st1.t);
    if (abs(denom) < 0.0001) {
        return N;
    }
    
    float r = 1.0 / denom;
    vec3 T  = normalize((Q1 * st2.t - Q2 * st1.t) * r);
    vec3 B  = normalize((Q2 * st1.s - Q1 * st2.s) * r);
    mat3 TBN = mat3(T, B, N);
    
    return normalize(TBN * tangentNormal);
}

void main() {
    vec3 albedo = u_Albedo;
    if (u_HasAlbedoTex == 1) {
        vec4 texColor = texture(u_AlbedoTex, v_TexCoord);
        albedo *= texColor.rgb;
    }
    
    float isEmissive = (u_EmissiveStrength > 0.5) ? 1.0 : 0.0;
    gAlbedo = vec4(albedo, isEmissive);
    
    vec3 finalNormal = getNormalFromMap(v_WorldPos, v_WorldNormal, v_TexCoord);
    vec2 octNormal = OctEncode(finalNormal);
    gNormal = vec4(octNormal, 0.0, 0.0);
    
    float metallic = u_Metallic;
    float roughness = u_Roughness;
    float ao = 1.0;
    if (u_HasMetallicRoughnessTex == 1) {
        vec3 pbrTex = texture(u_MetallicRoughnessTex, v_TexCoord).rgb;
        roughness *= pbrTex.g;
        metallic *= pbrTex.b;
    }
    gPBR = vec4(metallic, roughness, ao, u_EmissiveStrength);
    
    vec2 currentPos = (v_CurrentNDC.xy / v_CurrentNDC.w);
    vec2 prevPos = (v_PrevNDC.xy / v_PrevNDC.w);
    vec2 velocity = (currentPos - prevPos) * 0.5;
    gVelocity = velocity;
}
