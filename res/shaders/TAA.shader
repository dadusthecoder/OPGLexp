// =============================================================================
// TAA.shader
// Temporal Anti-Aliasing
// =============================================================================

#shader Vertex
#version 460 core

out vec2 v_TexCoord;

void main() {
    vec2 position = vec2(
        float((gl_VertexID << 1) & 2) * 2.0 - 1.0,
        float(gl_VertexID & 2) * 2.0 - 1.0
    );
    v_TexCoord = position * 0.5 + 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
}

#shader Fragment
#version 460 core

in vec2 v_TexCoord;
out vec4 FragColor;

layout(binding = 0) uniform sampler2D u_CurrentFrame;
layout(binding = 1) uniform sampler2D u_HistoryFrame;
layout(binding = 2) uniform sampler2D u_VelocityTexture;
layout(binding = 3) uniform sampler2D u_DepthTexture;

uniform float u_Feedback; // Typical 0.9 - 0.95

// -----------------------------------------------------------------------------
// Convert RGB to YCoCg for better neighborhood clipping
// -----------------------------------------------------------------------------
vec3 RGBToYCoCg(vec3 rgb) {
    float Y  = dot(rgb, vec3( 0.25, 0.5,  0.25));
    float Co = dot(rgb, vec3( 0.5,  0.0, -0.5));
    float Cg = dot(rgb, vec3(-0.25, 0.5, -0.25));
    return vec3(Y, Co, Cg);
}

vec3 YCoCgToRGB(vec3 ycocg) {
    float Y  = ycocg.x;
    float Co = ycocg.y;
    float Cg = ycocg.z;
    float R = Y + Co - Cg;
    float G = Y + Cg;
    float B = Y - Co - Cg;
    return vec3(R, G, B);
}

// -----------------------------------------------------------------------------
// Clip History to Neighborhood AABB
// -----------------------------------------------------------------------------
vec3 ClipAABB(vec3 aabbMin, vec3 aabbMax, vec3 p, vec3 q) {
    vec3 p_clip = 0.5 * (aabbMax + aabbMin);
    vec3 e_clip = 0.5 * (aabbMax - aabbMin) + 0.00000001;

    vec3 v_clip = q - p_clip;
    vec3 v_unit = v_clip.xyz / e_clip;
    vec3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

    if (ma_unit > 1.0) {
        return p_clip + v_clip / ma_unit;
    } else {
        return q; // point inside aabb
    }
}

void main() {
    // 1. Fetch current pixel velocity
    // Velocity is written as (current - previous) in UV space
    vec2 velocity = texture(u_VelocityTexture, v_TexCoord).rg;
    
    // Dilate velocity (use the velocity of the closest pixel in a 3x3 neighborhood)
    // This helps prevent blurring on the edges of moving silhouettes
    float minDepth = 1.0;
    vec2 closestVel = velocity;
    
    ivec2 texel = ivec2(gl_FragCoord.xy);
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            float d = texelFetch(u_DepthTexture, texel + ivec2(x,y), 0).r;
            if (d < minDepth) {
                minDepth = d;
                closestVel = texelFetch(u_VelocityTexture, texel + ivec2(x,y), 0).rg;
            }
        }
    }
    
    // 2. Find previous position
    vec2 prevUV = v_TexCoord - closestVel;
    
    // Fetch current color
    vec3 currentColor = texture(u_CurrentFrame, v_TexCoord).rgb;
    
    // If the reprojected UV is outside the screen, just return current color
    if (prevUV.x < 0.0 || prevUV.x > 1.0 || prevUV.y < 0.0 || prevUV.y > 1.0) {
        FragColor = vec4(currentColor, 1.0);
        return;
    }

    // 3. Fetch history color using bicubic sampling or linear (linear is cheaper)
    vec3 historyColor = texture(u_HistoryFrame, prevUV).rgb;
    
    // 4. Neighborhood Clamping (to prevent ghosting)
    vec3 m1 = vec3(0.0);
    vec3 m2 = vec3(0.0);
    
    vec3 nColor0 = textureOffset(u_CurrentFrame, v_TexCoord, ivec2(-1, -1)).rgb;
    vec3 nColor1 = textureOffset(u_CurrentFrame, v_TexCoord, ivec2( 0, -1)).rgb;
    vec3 nColor2 = textureOffset(u_CurrentFrame, v_TexCoord, ivec2( 1, -1)).rgb;
    vec3 nColor3 = textureOffset(u_CurrentFrame, v_TexCoord, ivec2(-1,  0)).rgb;
    vec3 nColor4 = currentColor;
    vec3 nColor5 = textureOffset(u_CurrentFrame, v_TexCoord, ivec2( 1,  0)).rgb;
    vec3 nColor6 = textureOffset(u_CurrentFrame, v_TexCoord, ivec2(-1,  1)).rgb;
    vec3 nColor7 = textureOffset(u_CurrentFrame, v_TexCoord, ivec2( 0,  1)).rgb;
    vec3 nColor8 = textureOffset(u_CurrentFrame, v_TexCoord, ivec2( 1,  1)).rgb;

    vec3 minColor = min(min(min(min(min(min(min(min(nColor0, nColor1), nColor2), nColor3), nColor4), nColor5), nColor6), nColor7), nColor8);
    vec3 maxColor = max(max(max(max(max(max(max(max(nColor0, nColor1), nColor2), nColor3), nColor4), nColor5), nColor6), nColor7), nColor8);

    // Convert to YCoCg for better clamping
    vec3 historyYCoCg = RGBToYCoCg(historyColor);
    vec3 currentYCoCg = RGBToYCoCg(currentColor);
    vec3 minYCoCg = RGBToYCoCg(minColor);
    vec3 maxYCoCg = RGBToYCoCg(maxColor);

    historyYCoCg = ClipAABB(minYCoCg, maxYCoCg, currentYCoCg, historyYCoCg);
    historyColor = YCoCgToRGB(historyYCoCg);

    // 5. Weighting and resolve
    // Modulate feedback based on movement. If velocity is high, lower feedback to reduce ghosting
    float velMag = length(closestVel);
    float weight = clamp(u_Feedback - (velMag * 100.0), 0.7, u_Feedback);
    
    vec3 resolvedColor = mix(currentColor, historyColor, weight);
    FragColor = vec4(resolvedColor, 1.0);
}
