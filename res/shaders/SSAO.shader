// =============================================================================
// SSAO.shader
// Screen-Space Ambient Occlusion
// Reconstructs view-space position and normal from depth buffer, then
// samples a hemisphere kernel to estimate local occlusion.
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

// Depth buffer and noise texture inputs
layout(binding = 0) uniform sampler2D u_DepthTexture;
layout(binding = 1) uniform sampler2D u_NoiseTexture;
layout(binding = 2) uniform sampler2D u_gNormalRoughness;

// Camera matrices
uniform mat4 u_Projection;
uniform mat4 u_InvProjection;
uniform mat4 u_View;

// SSAO parameters
uniform vec2  u_NoiseScale;              // screenSize / noiseTexSize (for tiling)
uniform float u_Radius    = 0.5;         // Hemisphere sampling radius in view space
uniform float u_Bias      = 0.025;       // Depth bias to reduce self-occlusion
uniform float u_Intensity = 1.0;         // Occlusion intensity exponent
uniform int   u_KernelSize = 32;         // Number of kernel samples to use

// Hemisphere sample kernel (tangent-space, pre-computed on CPU)
uniform vec3 u_Samples[64];

// -----------------------------------------------------------------------------
// Reconstruct view-space position from depth buffer
// Maps screen UV + depth back through inverse projection
// -----------------------------------------------------------------------------
vec3 ReconstructViewPos(vec2 uv, float depth)
{
    // Convert to NDC: [0,1] -> [-1,1]
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    // Transform from clip space to view space
    vec4 viewPos = u_InvProjection * clipPos;
    return viewPos.xyz / viewPos.w;
}

void main()
{
    // Sample depth and reconstruct view-space position
    float depth = texture(u_DepthTexture, v_TexCoord).r;

    // Skip SSAO for far plane / sky pixels
    if (depth >= 1.0)
    {
        FragColor = 1.0;
        return;
    }

    vec3 viewPos = ReconstructViewPos(v_TexCoord, depth);
    
    // Read world-space normal from G-Buffer and convert to view-space
    vec3 worldNormal = texture(u_gNormalRoughness, v_TexCoord).rgb;
    vec3 normal = normalize(mat3(u_View) * worldNormal);

    // Sample the noise texture (tiled across screen) for random rotation
    // This breaks the banding pattern from the limited kernel size
    vec3 randomVec = normalize(texture(u_NoiseTexture, v_TexCoord * u_NoiseScale).xyz * 2.0 - 1.0);

    // -------------------------------------------------------------------------
    // Construct TBN matrix to orient hemisphere kernel along the surface normal
    // Uses Gram-Schmidt process to orthogonalize tangent with respect to normal
    // -------------------------------------------------------------------------
    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    // -------------------------------------------------------------------------
    // Hemisphere kernel sampling
    // For each sample, check if surrounding geometry occludes the point
    // -------------------------------------------------------------------------
    float occlusion = 0.0;
    int clampedKernelSize = min(u_KernelSize, 64);

    for (int i = 0; i < clampedKernelSize; ++i)
    {
        // Transform kernel sample from tangent space to view space
        vec3 samplePos = TBN * u_Samples[i];
        samplePos = viewPos + samplePos * u_Radius;

        // Project sample position to screen space to get its UV
        vec4 offset = u_Projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;                    // Perspective divide
        offset.xyz  = offset.xyz * 0.5 + 0.5;      // NDC [-1,1] -> UV [0,1]

        // Sample the depth buffer at the projected position
        float sampleDepth = texture(u_DepthTexture, offset.xy).r;
        vec3 sampleViewPos = ReconstructViewPos(offset.xy, sampleDepth);

        // Range check: only consider occlusion from nearby geometry
        // This prevents distant surfaces from incorrectly occluding
        float rangeCheck = smoothstep(0.0, 1.0, u_Radius / abs(viewPos.z - sampleViewPos.z));

        // Occlusion test: sample is occluded if the depth buffer surface
        // is closer than the sample point (with bias to prevent self-occlusion)
        occlusion += (sampleViewPos.z >= samplePos.z + u_Bias ? 1.0 : 0.0) * rangeCheck;
    }

    // Normalize and apply intensity
    // Output 1.0 = no occlusion, 0.0 = fully occluded
    FragColor = pow(1.0 - (occlusion / float(clampedKernelSize)), u_Intensity);
}
