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

// OpenGL 4.6 provides gl_BaseInstance which we set to the instanceID in the compute shader!
uniform mat4 u_ViewProjection;

out vec3 v_FragPos;
out vec3 v_Normal;
out vec2 v_TexCoord;

void main() {
    uint baseIndex = gl_VertexID * 8;
    vec3 vPos = vec3(vertices[baseIndex], vertices[baseIndex+1], vertices[baseIndex+2]);
    vec3 vNorm = vec3(vertices[baseIndex+3], vertices[baseIndex+4], vertices[baseIndex+5]);
    vec2 vTex = vec2(vertices[baseIndex+6], vertices[baseIndex+7]);
    
    mat4 model = instances[gl_BaseInstance].Transform;
    
    vec4 worldPos = model * vec4(vPos, 1.0);
    v_FragPos = worldPos.xyz;
    v_Normal = mat3(transpose(inverse(model))) * vNorm;
    v_TexCoord = vTex;
    gl_Position = u_ViewProjection * worldPos;
}

#type fragment
#version 460 core

layout(location = 0) out vec4 gAlbedoSpec;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gPBR;

struct Material {
    vec3 Albedo;
    float Metallic;
    float Roughness;
};

uniform Material u_Material;
uniform sampler2D u_AlbedoMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_MetallicMap;
uniform sampler2D u_RoughnessMap;

uniform int u_UseAlbedoMap;
uniform int u_UseNormalMap;
uniform int u_UseMetallicMap;
uniform int u_UseRoughnessMap;

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;

void main() {
    vec3 albedo = u_Material.Albedo;
    if (u_UseAlbedoMap == 1) {
        albedo *= texture(u_AlbedoMap, v_TexCoord).rgb;
    }
    
    vec3 normal = normalize(v_Normal);
    if (u_UseNormalMap == 1) {
        // Sample normal map and transform from tangent space [0,1] to [-1,1]
        vec3 sampledNormal = texture(u_NormalMap, v_TexCoord).rgb * 2.0 - 1.0;
        // For proper tangent-space normal mapping we need TBN matrix,
        // but as a simple approximation we perturb the geometric normal
        normal = normalize(normal + sampledNormal * 0.5);
    }
    
    float metallic = u_Material.Metallic;
    if (u_UseMetallicMap == 1) {
        metallic *= texture(u_MetallicMap, v_TexCoord).r;
    }
    
    float roughness = u_Material.Roughness;
    if (u_UseRoughnessMap == 1) {
        roughness *= texture(u_RoughnessMap, v_TexCoord).r;
    }
    
    gAlbedoSpec = vec4(albedo, 1.0);
    gNormal = vec4(normal, 1.0);
    gPBR = vec4(metallic, roughness, 1.0, 1.0); // B=AO
}
