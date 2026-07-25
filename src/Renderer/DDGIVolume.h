#pragma once
#include "Vendor/glad.h"
#include <glm/glm.hpp>
#include "Helpers/Logger.h"

namespace lgt {

// ============================================================
// DDGIVolume — Manages a 3D grid of irradiance probes
//
// Each probe stores:
//   - Irradiance (8x8 octahedral texels in a 2D atlas, RGBA16F)
//   - Depth + depth² (16x16 octahedral texels, RG16F) [future]
//
// The atlas packs all probes into a single 2D texture with
// 1px borders for bilinear filtering.
// ============================================================
class DDGIVolume {
public:
    // Grid configuration
    glm::vec3  origin  = glm::vec3(0.0f);
    glm::vec3  spacing = glm::vec3(2.0f);
    glm::ivec3 probeCount = glm::ivec3(12, 8, 12);

    // Probe settings
    int   raysPerProbe    = 64;
    float hysteresis      = 0.97f;
    float maxRayDistance   = 50.0f;
    float normalBias      = 0.25f;
    float giIntensity     = 1.0f;
    float giBlend         = 0.8f;

    // Atlas textures
    GLuint irradianceAtlas = 0;  // GL_RGBA16F
    GLuint depthAtlas      = 0;  // GL_RG16F (future)

    // Probe ray data SSBO (intermediate between trace and update)
    GLuint rayDataSSBO     = 0;  // Binding 6

    // Atlas dimensions
    static constexpr int IRRADIANCE_TEXELS = 8;   // per probe
    static constexpr int DEPTH_TEXELS      = 16;  // per probe
    static constexpr int BORDER            = 1;    // border pixels for filtering

    void Init(glm::vec3 sceneMin, glm::vec3 sceneMax) {
        // Auto-fit grid to scene bounds with some padding
        glm::vec3 sceneSize = sceneMax - sceneMin;
        glm::vec3 padding = sceneSize * 0.1f;
        origin = sceneMin - padding;
        glm::vec3 extendedSize = sceneSize + padding * 2.0f;

        // Compute spacing from probe count
        spacing = extendedSize / glm::vec3(probeCount - glm::ivec3(1));

        CreateResources();

        CORE_INFO("DDGIVolume initialized: {}x{}x{} probes, spacing ({:.2f}, {:.2f}, {:.2f})",
                  probeCount.x, probeCount.y, probeCount.z,
                  spacing.x, spacing.y, spacing.z);
        CORE_INFO("  Origin: ({:.2f}, {:.2f}, {:.2f})", origin.x, origin.y, origin.z);
        CORE_INFO("  Irradiance atlas: {}x{}", IrradianceAtlasWidth(), IrradianceAtlasHeight());
    }

    void Shutdown() {
        if (irradianceAtlas) { glDeleteTextures(1, &irradianceAtlas); irradianceAtlas = 0; }
        if (depthAtlas)      { glDeleteTextures(1, &depthAtlas);      depthAtlas = 0; }
        if (rayDataSSBO)     { glDeleteBuffers(1, &rayDataSSBO);      rayDataSSBO = 0; }
    }

    int TotalProbes() const {
        return probeCount.x * probeCount.y * probeCount.z;
    }

    glm::vec3 ProbePosition(glm::ivec3 idx) const {
        return origin + glm::vec3(idx) * spacing;
    }

    glm::vec3 ProbePosition(int linearIndex) const {
        glm::ivec3 idx;
        idx.x = linearIndex % probeCount.x;
        idx.y = (linearIndex / probeCount.x) % probeCount.y;
        idx.z = linearIndex / (probeCount.x * probeCount.y);
        return ProbePosition(idx);
    }

    int IrradianceAtlasWidth() const {
        return probeCount.x * (IRRADIANCE_TEXELS + 2 * BORDER);
    }

    int IrradianceAtlasHeight() const {
        int probesPerRow = probeCount.x;
        int totalRows = probeCount.y * probeCount.z;
        (void)probesPerRow;
        return totalRows * (IRRADIANCE_TEXELS + 2 * BORDER);
    }

    // Apply quality preset
    void ApplyPreset(int preset) {
        switch (preset) {
            case 0: // Low
                probeCount = glm::ivec3(8, 4, 8);
                raysPerProbe = 16;
                break;
            case 1: // Medium
                probeCount = glm::ivec3(10, 6, 10);
                raysPerProbe = 32;
                break;
            case 2: // High
                probeCount = glm::ivec3(12, 8, 12);
                raysPerProbe = 64;
                break;
            case 3: // Ultra
                probeCount = glm::ivec3(16, 10, 16);
                raysPerProbe = 128;
                break;
        }
    }

private:
    void CreateResources() {
        // --- Irradiance Atlas (RGBA16F) ---
        int atlasW = IrradianceAtlasWidth();
        int atlasH = IrradianceAtlasHeight();

        glGenTextures(1, &irradianceAtlas);
        glBindTexture(GL_TEXTURE_2D, irradianceAtlas);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, atlasW, atlasH, 0,
                     GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Clear to black
        std::vector<float> zeros(atlasW * atlasH * 4, 0.0f);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, atlasW, atlasH,
                        GL_RGBA, GL_FLOAT, zeros.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        // --- Depth Atlas (RG16F) — future ---
        // For now, skip depth atlas. Chebyshev visibility test
        // will be added in a follow-up pass.

        // --- Ray Data SSBO ---
        // Each probe traces raysPerProbe rays. Each ray stores:
        //   vec4 radiance (16 bytes) + vec4 direction (16 bytes) = 32 bytes
        size_t rayDataSize = static_cast<size_t>(TotalProbes()) * raysPerProbe * 32;
        glGenBuffers(1, &rayDataSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, rayDataSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, rayDataSize, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        CORE_INFO("  Ray data SSBO: {} bytes ({} probes x {} rays x 32B)",
                  rayDataSize, TotalProbes(), raysPerProbe);
    }
};

} // namespace lgt
