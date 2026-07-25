// =============================================================================
// BloomExtract.shader
// Extracts bright pixels from HDR buffer for bloom processing
// Uses soft-knee thresholding for smooth transition at the threshold boundary
// =============================================================================

#shader Vertex
#version 460 core

// Fullscreen triangle - no vertex attributes needed
out vec2 v_TexCoord;

void main()
{
    // Generate fullscreen triangle vertices from gl_VertexID
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

// HDR color buffer input
layout(binding = 0) uniform sampler2D u_HDRBuffer;

// Bloom threshold controls
uniform float u_Threshold = 1.0;   // Brightness threshold for bloom
uniform float u_SoftKnee  = 0.5;   // Soft threshold knee [0..1], 0 = hard cutoff

void main()
{
    vec3 color = texture(u_HDRBuffer, v_TexCoord).rgb;

    // Compute brightness as max of RGB channels
    float brightness = max(max(color.r, color.g), color.b);

    // Soft thresholding - creates a smooth transition at the threshold
    // instead of a hard cutoff that causes flickering/popping artifacts
    float knee = u_Threshold * u_SoftKnee;
    float soft = brightness - u_Threshold + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.00001);

    // Final contribution: blend between soft and hard threshold
    float contribution = max(soft, brightness - u_Threshold) / max(brightness, 0.00001);

    FragColor = vec4(color * contribution, 1.0);
}
