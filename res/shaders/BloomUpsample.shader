// =============================================================================
// BloomUpsample.shader
// Progressive upsample for bloom using a 9-tap tent filter
// Each mip level is upsampled and additively blended with the next level up
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

// Source texture (lower mip to upsample)
layout(binding = 0) uniform sampler2D u_SourceTexture;

// Filter radius in UV space (typically texelSize * filterScale)
uniform vec2 u_FilterRadius;

void main()
{
    vec2 uv = v_TexCoord;

    // -------------------------------------------------------------------------
    // 9-tap tent filter for upsampling
    // This produces a smooth, tent-shaped kernel that blends nicely when
    // progressively upsampling the bloom mip chain.
    //
    // Sample pattern and weights:
    //   1  2  1
    //   2  4  2   / 16
    //   1  2  1
    // -------------------------------------------------------------------------
    float x = u_FilterRadius.x;
    float y = u_FilterRadius.y;

    // Sample the 3x3 neighborhood
    vec3 a = texture(u_SourceTexture, vec2(uv.x - x, uv.y + y)).rgb;  // top-left
    vec3 b = texture(u_SourceTexture, vec2(uv.x,     uv.y + y)).rgb;  // top-center
    vec3 c = texture(u_SourceTexture, vec2(uv.x + x, uv.y + y)).rgb;  // top-right

    vec3 d = texture(u_SourceTexture, vec2(uv.x - x, uv.y)).rgb;      // mid-left
    vec3 e = texture(u_SourceTexture, vec2(uv.x,     uv.y)).rgb;      // center
    vec3 f = texture(u_SourceTexture, vec2(uv.x + x, uv.y)).rgb;      // mid-right

    vec3 g = texture(u_SourceTexture, vec2(uv.x - x, uv.y - y)).rgb;  // bot-left
    vec3 h = texture(u_SourceTexture, vec2(uv.x,     uv.y - y)).rgb;  // bot-center
    vec3 i = texture(u_SourceTexture, vec2(uv.x + x, uv.y - y)).rgb;  // bot-right

    // Apply tent filter weights
    vec3 upsample = e * 4.0;              // Center: weight 4
    upsample += (b + d + f + h) * 2.0;    // Edges:  weight 2 each
    upsample += (a + c + g + i);           // Corners: weight 1 each
    upsample *= 1.0 / 16.0;               // Normalize (4 + 4*2 + 4*1 = 16)

    FragColor = vec4(upsample, 1.0);
}
