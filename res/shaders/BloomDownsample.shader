// =============================================================================
// BloomDownsample.shader
// Progressive downsample for bloom mip chain generation
// Uses the Jimenez 2014 13-tap filter (Call of Duty: Advanced Warfare)
// Reference: "Next Generation Post Processing in Call of Duty: Advanced Warfare"
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

// Source texture to downsample
layout(binding = 0) uniform sampler2D u_SourceTexture;

// Resolution of the source texture (needed for texel size calculation)
uniform vec2 u_SrcResolution;

void main()
{
    vec2 uv = v_TexCoord;

    // -------------------------------------------------------------------------
    // Jimenez 2014 - 13-tap downsample filter
    // This filter produces better quality than a naive bilinear downsample by
    // sampling in a pattern that reduces aliasing and preserves more detail.
    //
    // Sample pattern (letters correspond to sample positions):
    //   a . b . c
    //   . j . k .
    //   d . e . f
    //   . l . m .
    //   g . h . i
    // -------------------------------------------------------------------------
    vec2 texelSize = 1.0 / u_SrcResolution;
    float x = texelSize.x;
    float y = texelSize.y;

    // Take 13 samples around the current texel 'e'
    // Outer ring (corners at 2-texel distance)
    vec3 a = texture(u_SourceTexture, vec2(uv.x - 2*x, uv.y + 2*y)).rgb;
    vec3 b = texture(u_SourceTexture, vec2(uv.x,       uv.y + 2*y)).rgb;
    vec3 c = texture(u_SourceTexture, vec2(uv.x + 2*x, uv.y + 2*y)).rgb;

    vec3 d = texture(u_SourceTexture, vec2(uv.x - 2*x, uv.y)).rgb;
    vec3 e = texture(u_SourceTexture, vec2(uv.x,       uv.y)).rgb;
    vec3 f = texture(u_SourceTexture, vec2(uv.x + 2*x, uv.y)).rgb;

    vec3 g = texture(u_SourceTexture, vec2(uv.x - 2*x, uv.y - 2*y)).rgb;
    vec3 h = texture(u_SourceTexture, vec2(uv.x,       uv.y - 2*y)).rgb;
    vec3 i = texture(u_SourceTexture, vec2(uv.x + 2*x, uv.y - 2*y)).rgb;

    // Inner ring (at 1-texel distance)
    vec3 j = texture(u_SourceTexture, vec2(uv.x - x, uv.y + y)).rgb;
    vec3 k = texture(u_SourceTexture, vec2(uv.x + x, uv.y + y)).rgb;
    vec3 l = texture(u_SourceTexture, vec2(uv.x - x, uv.y - y)).rgb;
    vec3 m = texture(u_SourceTexture, vec2(uv.x + x, uv.y - y)).rgb;

    // Weighted combination:
    // Center sample 'e': weight 0.125 (2/16)
    // Corner samples (a,c,g,i): weight 0.03125 each (0.5/16)
    // Edge samples (b,d,f,h): weight 0.0625 each (1/16)
    // Inner samples (j,k,l,m): weight 0.125 each (2/16)
    vec3 downsample = e * 0.125;
    downsample += (a + c + g + i) * 0.03125;
    downsample += (b + d + f + h) * 0.0625;
    downsample += (j + k + l + m) * 0.125;

    FragColor = vec4(downsample, 1.0);
}
