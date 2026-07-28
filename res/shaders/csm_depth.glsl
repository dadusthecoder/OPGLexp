#type vertex
#version 460 core

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

uniform mat4 u_LightSpaceMatrix;

void main() {
    uint baseIndex = gl_VertexID * 8;
    vec3 vPos = vec3(vertices[baseIndex], vertices[baseIndex+1], vertices[baseIndex+2]);
    mat4 model = instances[gl_BaseInstance].Transform;
    gl_Position = u_LightSpaceMatrix * model * vec4(vPos, 1.0);
}

#type fragment
#version 460 core

void main() {
    // Empty fragment shader, only writing depth
}
