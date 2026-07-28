#type vertex
#version 460 core

out vec2 v_TexCoord;

void main() {
    // Fullscreen triangle (no VBO needed, attribute-less rendering)
    vec2 positions[3] = vec2[](vec2(-1,-1), vec2(3,-1), vec2(-1,3));
    vec2 uvs[3]       = vec2[](vec2(0,0),  vec2(2,0),  vec2(0,2));
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
    v_TexCoord  = uvs[gl_VertexID];
}


#type fragment
#version 460 core
#include "include/common.glsl"

in vec2 v_TexCoord;
out vec4 o_Color;

uniform sampler2D u_CurrentColor;  // slot 0
uniform sampler2D u_HistoryColor;  // slot 1
uniform sampler2D u_Velocity;      // slot 2 (RG16F velocity in NDC space)
uniform sampler2D u_Depth;         // slot 3
uniform vec2      u_Resolution;
uniform float     u_BlendFactor;   // 0.1 = 10% current, 90% history

// Convert RGB to YCoCg for neighbourhood clamping
vec3 RGBtoYCoCg(vec3 c) {
    return vec3(
         0.25*c.r + 0.5*c.g + 0.25*c.b,
         0.5*c.r              - 0.5*c.b,
        -0.25*c.r + 0.5*c.g - 0.25*c.b
    );
}
vec3 YCoCgtoRGB(vec3 c) {
    return vec3(
        c.x + c.y - c.z,
        c.x + c.z,
        c.x - c.y - c.z
    );
}

void main() {
    vec2 texelSize = 1.0 / u_Resolution;
    vec2 velocity  = texture(u_Velocity, v_TexCoord).rg;
    vec2 historyUV = v_TexCoord - velocity;

    vec3 current = texture(u_CurrentColor, v_TexCoord).rgb;
    vec3 history = texture(u_HistoryColor, historyUV).rgb;

    // Build neighbourhood AABB in YCoCg space (3x3 cross pattern)
    vec3 cYCoCg = RGBtoYCoCg(current);
    vec3 nMin = cYCoCg, nMax = cYCoCg;

    vec2 offsets[4] = vec2[](vec2(-1,0), vec2(1,0), vec2(0,-1), vec2(0,1));
    for (int i = 0; i < 4; i++) {
        vec3 s = RGBtoYCoCg(texture(u_CurrentColor, v_TexCoord + offsets[i] * texelSize).rgb);
        nMin = min(nMin, s);
        nMax = max(nMax, s);
    }

    // Clamp history to neighbourhood
    vec3 hYCoCg  = RGBtoYCoCg(history);
    vec3 clamped = clamp(hYCoCg, nMin, nMax);
    history = YCoCgtoRGB(clamped);

    // Disocclusion check: if reprojection out of screen, use current
    if (historyUV.x < 0.0 || historyUV.x > 1.0 || historyUV.y < 0.0 || historyUV.y > 1.0) {
        o_Color = vec4(current, 1.0);
        return;
    }

    o_Color = vec4(mix(history, current, u_BlendFactor), 1.0);
}
