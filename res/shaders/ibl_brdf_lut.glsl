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

#include "include/common.glsl"
#include "include/sampling.glsl"
#include "include/brdf.glsl"

in vec2 v_TexCoord;
out vec2 o_Color;

void main() {
    float NdotV = max(v_TexCoord.x, 0.001);
    float roughness = max(v_TexCoord.y, 0.001);
    
    vec2 integratedBRDF = IntegrateBRDF(NdotV, roughness, 512u);
    o_Color = integratedBRDF;
}
