#pragma once
#include "RenderPass.h"
#include "Scene.h"
#include "Shader.h"
#include "DDGIVolume.h"
#include "RayTracer.h"
#include "AccelerationStructure.h"

namespace lgt {

// ============================================================
// DDGIPass — Dynamic Diffuse Global Illumination
//
// Traces rays from a grid of probes and updates their irradiance
// atlases. Uses a basic scheduler to amortize costs over frames.
// ============================================================
class DDGIPass : public RenderPass {
public:
    DDGIPass() = default;
    ~DDGIPass() override = default;

    void Init(RenderContext& ctx) override {
        m_traceShader  = new Pipeline("res/shaders/DDGIProbeTrace.comp", ShaderType::COMPUTESHADER);
        m_updateShader = new Pipeline("res/shaders/DDGIProbeUpdate.comp", ShaderType::COMPUTESHADER);
        m_borderShader = new Pipeline("res/shaders/DDGIBorderUpdate.comp", ShaderType::COMPUTESHADER);

        if (!m_traceShader || !m_traceShader->isValid())
            CORE_ERROR("DDGIPass: Failed to compile DDGIProbeTrace.comp");
        if (!m_updateShader || !m_updateShader->isValid())
            CORE_ERROR("DDGIPass: Failed to compile DDGIProbeUpdate.comp");
        if (!m_borderShader || !m_borderShader->isValid())
            CORE_ERROR("DDGIPass: Failed to compile DDGIBorderUpdate.comp");

        CORE_INFO("DDGIPass initialized");
    }

    void Execute(RenderContext& ctx) override {
        if (!m_traceShader || !m_traceShader->isValid()) return;
        if (!m_updateShader || !m_updateShader->isValid()) return;
        if (!m_borderShader || !m_borderShader->isValid()) return;
        if (!ctx.ddgi.enabled) return;

        auto* scene = ctx.scene;
        if (!scene) return;

        auto* accel = scene->GetAccelerationStructure();
        if (!accel || !accel->IsBuilt()) return;

        auto& volumes = scene->GetProbeVolumes();
        if (volumes.empty()) return;

        // --- Execute for each volume ---
        for (auto& volume : volumes) {
            if (!volume) continue;

            // Apply settings from UI
            volume->giIntensity = ctx.ddgi.giIntensity;
            volume->giBlend     = ctx.ddgi.giBlend;
            volume->hysteresis  = ctx.ddgi.hysteresis;
            volume->maxRayDistance = ctx.ddgi.maxRayDist;
            volume->normalBias  = ctx.ddgi.normalBias;

            // Simple update: for now, update all probes every frame.
            // A more advanced scheduler can be added later to update a subset.
            int totalProbes = volume->TotalProbes();

            // 1. Trace Rays
            m_traceShader->use();
            accel->Bind();

            // Bind ray data SSBO
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, volume->rayDataSSBO);

            // Set uniforms
            m_traceShader->setIVec3("u_ProbeCount", volume->probeCount);
            m_traceShader->setVec3("u_ProbeOrigin", volume->origin);
            m_traceShader->setVec3("u_ProbeSpacing", volume->spacing);
            m_traceShader->setFloat("u_MaxRayDistance", volume->maxRayDistance);
            m_traceShader->setInt("u_FrameCounter", m_frameCounter++);
            m_traceShader->setInt("u_RaysPerProbe", volume->raysPerProbe);

            // Mock sun (since we don't have direct light access in this pass yet)
            if (ctx.scene && !ctx.scene->getLights().empty()) {
                auto& sun = ctx.scene->getLights()[0];
                m_traceShader->setVec3("u_SunDirection", glm::normalize(-glm::vec3(sun.direction)));
                m_traceShader->setVec3("u_SunColor", glm::vec3(sun.color) * sun.color.w);
            } else {
                m_traceShader->setVec3("u_SunDirection", glm::normalize(glm::vec3(-0.2f, 1.0f, -0.3f)));
                m_traceShader->setVec3("u_SunColor", glm::vec3(1.0f));
            }
            
            // Bind Skybox (Irradiance map for ambient)
            if (ctx.irradianceMap > 0) {
                glActiveTexture(GL_TEXTURE0 + 10);
                glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.irradianceMap);
                m_traceShader->setInt("u_Skybox", 10);
            }
            
            // Dispatch trace: 1 workgroup per probe
            m_traceShader->dispatch(totalProbes, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            accel->Unbind();

            // 2. Update Irradiance & Depth Atlas
            m_updateShader->use();
            glBindImageTexture(0, volume->irradianceAtlas, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
            glBindImageTexture(1, volume->depthAtlas, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG16F);
            
            // Re-bind SSBO just to be safe
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, volume->rayDataSSBO);

            m_updateShader->setIVec3("u_ProbeCount", volume->probeCount);
            m_updateShader->setInt("u_RaysPerProbe", volume->raysPerProbe);
            m_updateShader->setFloat("u_Hysteresis", volume->hysteresis);
            m_updateShader->setInt("u_IrradianceTexSize", DDGIVolume::IRRADIANCE_TEXELS);

            // Dispatch update: 1 workgroup per probe (16x16 threads is enough for both 8x8 irradiance and 16x16 depth)
            m_updateShader->dispatch(volume->probeCount.x, volume->probeCount.y, volume->probeCount.z);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

            // 3. Update Atlas Borders
            m_borderShader->use();
            glBindImageTexture(0, volume->irradianceAtlas, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
            glBindImageTexture(1, volume->depthAtlas, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG16F);
            
            m_borderShader->setIVec3("u_ProbeCount", volume->probeCount);
            m_borderShader->setInt("u_TexSize", DDGIVolume::IRRADIANCE_TEXELS);
            
            // Dispatch border update: 1 workgroup per probe, threads mapped to border pixels
            m_borderShader->dispatch(volume->probeCount.x, volume->probeCount.y, volume->probeCount.z);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

            // Set the atlas in the context so the lighting pass can read it
            ctx.gpuResources->ddgiIrradianceAtlas = volume->irradianceAtlas;
            ctx.gpuResources->ddgiDepthAtlas = volume->depthAtlas;
            
            // Pass the first volume's settings to context for the lighting shader
            ctx.ddgi.giIntensity = volume->giIntensity;
            ctx.ddgi.giBlend = volume->giBlend;
        }
    }

    void Shutdown() override {
        delete m_traceShader;
        delete m_updateShader;
        delete m_borderShader;
        m_traceShader = nullptr;
        m_updateShader = nullptr;
        m_borderShader = nullptr;
    }

    const char* GetName() const override { return "DDGIPass"; }

private:
    Pipeline* m_traceShader  = nullptr;
    Pipeline* m_updateShader = nullptr;
    Pipeline* m_borderShader = nullptr;
    int       m_frameCounter = 0;
};

} // namespace lgt
