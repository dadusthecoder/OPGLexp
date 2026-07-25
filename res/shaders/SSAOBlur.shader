// =============================================================================
// SSAOBlur.shader
// Simple 4x4 box blur to smooth SSAO output
// Removes the noise pattern introduced by the limited kernel size and
// randomized rotation in the SSAO pass
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
out float FragColor;

// SSAO texture to blur
layout(binding = 0) uniform sampler2D u_SSAOTexture;

void main()
{
    // Compute texel size from texture dimensions
    vec2 texelSize = 1.0 / vec2(textureSize(u_SSAOTexture, 0));

    // -------------------------------------------------------------------------
    // 4x4 box blur
    // Averages a 4x4 neighborhood centered around the current fragment
    // The offset range [-1.5, 1.5] centers the 4x4 kernel on the texel
    // -------------------------------------------------------------------------
    float result = 0.0;

    for (int x = -2; x < 2; ++x)
    {
        for (int y = -2; y < 2; ++y)
        {
            vec2 offset = vec2(float(x) + 0.5, float(y) + 0.5) * texelSize;
            result += texture(u_SSAOTexture, v_TexCoord + offset).r;
        }
    }

    // Average over 16 samples (4x4)
    FragColor = result / 16.0;
}
