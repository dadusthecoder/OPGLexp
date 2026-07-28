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

uniform sampler2D u_HDRBuffer;
uniform sampler2D u_BloomBuffer;
uniform float u_Exposure = 1.0;

// ACES Tonemapping curve
vec3 ACESFilm(vec3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main() {
    vec3 hdrColor = texture(u_HDRBuffer, v_TexCoord).rgb;
    vec3 bloomColor = texture(u_BloomBuffer, v_TexCoord).rgb;
    
    // Add bloom
    hdrColor += bloomColor;
    
    // Exposure adjustment
    hdrColor *= u_Exposure;

    // ACES Tone Mapping (HDR -> LDR)
    vec3 ldrColor = ACESFilm(hdrColor);
    
    // Gamma Correction (LDR -> sRGB)
    const float gamma = 2.2;
    vec3 srgbColor = pow(ldrColor, vec3(1.0 / gamma));
    
    o_Color = vec4(srgbColor, 1.0);
}
