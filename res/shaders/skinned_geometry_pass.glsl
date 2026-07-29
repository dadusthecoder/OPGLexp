#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in uvec4 a_BoneIDs;
layout(location = 4) in vec4 a_BoneWeights;

layout(std430, binding = 6) readonly buffer SkinMatrixBuffer {
    mat4 u_BoneMatrices[];
};

uniform mat4 u_Model;
uniform mat4 u_ViewProjection;

out vec3 v_FragPos;
out vec3 v_Normal;
out vec2 v_TexCoord;

void main() {
    mat4 skinMatrix = a_BoneWeights.x * u_BoneMatrices[a_BoneIDs.x] +
                      a_BoneWeights.y * u_BoneMatrices[a_BoneIDs.y] +
                      a_BoneWeights.z * u_BoneMatrices[a_BoneIDs.z] +
                      a_BoneWeights.w * u_BoneMatrices[a_BoneIDs.w];
    
    // Fallback if weights are zero (e.g., unskinned vertex in a skinned mesh)
    if (a_BoneWeights.x + a_BoneWeights.y + a_BoneWeights.z + a_BoneWeights.w < 0.01) {
        skinMatrix = mat4(1.0);
    }

    mat4 modelSkin = u_Model * skinMatrix;

    vec4 worldPos = modelSkin * vec4(a_Position, 1.0);
    v_FragPos = worldPos.xyz;
    
    // Normal matrix
    v_Normal = mat3(transpose(inverse(modelSkin))) * a_Normal;
    
    v_TexCoord = a_TexCoord;
    
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
        vec3 sampledNormal = texture(u_NormalMap, v_TexCoord).rgb * 2.0 - 1.0;
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
