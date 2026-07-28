#type vertex
#version 460 core
out vec2 v_TexCoord;
void main() {
    vec2 pos[3] = vec2[](vec2(-1,-1), vec2(3,-1), vec2(-1,3));
    vec2 uvs[3] = vec2[](vec2(0,0),  vec2(2,0),  vec2(0,2));
    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
    v_TexCoord  = uvs[gl_VertexID];
}

#type fragment
#version 460 core
#include "include/common.glsl"

in  vec2 v_TexCoord;
out vec4 o_Color;

uniform sampler2D u_HDRBuffer;
uniform sampler2D u_BloomTexture;
uniform float u_Exposure;

vec3 ACESFilm(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main() {
    vec3 hdr = texture(u_HDRBuffer, v_TexCoord).rgb;
    vec3 bloom = texture(u_BloomTexture, v_TexCoord).rgb;
    
    hdr += bloom;
    
    vec3 exposed  = hdr * u_Exposure;
    vec3 tonemapped = ACESFilm(exposed);
    vec3 gamma = pow(tonemapped, vec3(1.0 / 2.2));
    o_Color = vec4(gamma, 1.0);
}
