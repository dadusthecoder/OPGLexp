// =============================================================================
// ToneMap.shader
// Fullscreen post-processing: ACES filmic tone mapping with exposure & gamma
// =============================================================================

#shader Vertex
#version 460 core

// Fullscreen triangle - no vertex attributes needed
// Generates a triangle that covers the entire screen using gl_VertexID

out vec2 v_TexCoord;

void main()
{
    // Generate fullscreen triangle vertices from gl_VertexID
    // Vertex 0: (-1, -1), Vertex 1: (3, -1), Vertex 2: (-1, 3)
    vec2 position = vec2(
        float((gl_VertexID << 1) & 2) * 2.0 - 1.0,
        float(gl_VertexID & 2) * 2.0 - 1.0
    );

    // Map from [-1,1] NDC to [0,1] UV space
    v_TexCoord = position * 0.5 + 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
}

#shader Fragment
#version 460 core

in vec2 v_TexCoord;
out vec4 FragColor;

// HDR color buffer input
layout(binding = 0) uniform sampler2D u_HDRBuffer;

// Bloom texture (combined bloom mip chain result)
layout(binding = 1) uniform sampler2D u_BloomTexture;

// Tone mapping controls
uniform float u_Exposure     = 1.0;    // Exposure multiplier
uniform float u_Gamma        = 2.2;    // Gamma correction value
uniform float u_WhitePoint   = 1.0;    // White point for tone mapping
uniform bool  u_BloomEnabled = false;  // Whether bloom is active
uniform float u_BloomStrength = 0.5;   // Bloom mix intensity

// AO texture (for debug visualization)
uniform bool      u_ShowAO = false;
layout(binding = 2) uniform sampler2D u_AOTexture;

// -----------------------------------------------------------------------------
// ACES Filmic Tone Mapping Curve
// Standard Stephen Hill fit (s-curve approximation of the ACES RRT/ODT)
// Reference: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
// -----------------------------------------------------------------------------
vec3 ACESFilm(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    // Debug: show AO only
    if (u_ShowAO) {
        float ao = texture(u_AOTexture, v_TexCoord).r;
        FragColor = vec4(vec3(ao), 1.0);
        return;
    }

    // Sample the HDR color buffer
    vec3 hdrColor = texture(u_HDRBuffer, v_TexCoord).rgb;

    // Composite bloom (additive, before tone mapping so bloom operates in HDR)
    if (u_BloomEnabled) {
        vec3 bloomColor = texture(u_BloomTexture, v_TexCoord).rgb;
        hdrColor += bloomColor * u_BloomStrength;
    }

    // Apply exposure
    vec3 exposed = hdrColor * u_Exposure;

    // Apply ACES filmic tone mapping
    vec3 tonemapped = ACESFilm(exposed);
    
    // Normalize by white point
    vec3 whiteScale = 1.0 / ACESFilm(vec3(u_WhitePoint));
    tonemapped = clamp(tonemapped * whiteScale, 0.0, 1.0);

    // Apply gamma correction (linear -> sRGB approximation)
    vec3 gammaCorrected = pow(tonemapped, vec3(1.0 / u_Gamma));

    FragColor = vec4(gammaCorrected, 1.0);
}
