#pragma once
#include "RenderPass.h"
#include "Scene.h"
#include "Shader.h"
#include "RayTracer.h"
#include "RayTracer.h"
#include "AccelerationStructure.h"
#include "GPUResources.h"

namespace lgt {

// ============================================================
// RTAOPass — Ray-Traced Ambient Occlusion
//
// Inherits AmbientOcclusionPass (same base as SSAOPass) so the
// renderer can swap between SSAO and RTAO at runtime.
//
// Pipeline:
//   1. Dispatch RTAO.comp (1 ray per pixel, half or full res)
//   2. Temporal accumulation via TemporalFramework
//   3. Dispatch RTAODenoise.comp (bilateral blur)
//   4. Result written to ctx.gpuResources->aoTexture
// ============================================================
class RTAOPass : public AmbientOcclusionPass {
public:
    RTAOPass() = default;
    ~RTAOPass() override = default;

    void Init(RenderContext& ctx) override {
        m_traceShader   = new Pipeline("res/shaders/RTAO.comp", ShaderType::COMPUTESHADER);
        m_denoiseShader = new Pipeline("res/shaders/RTAODenoise.comp", ShaderType::COMPUTESHADER);

        if (!m_traceShader || !m_traceShader->isValid())
            CORE_ERROR("RTAOPass: Failed to compile RTAO.comp");
        if (!m_denoiseShader || !m_denoiseShader->isValid())
            CORE_ERROR("RTAOPass: Failed to compile RTAODenoise.comp");

        // Create raw AO texture (half-res R16F for noisy 1spp output)
        m_halfWidth  = ctx.screenWidth / 2;
        m_halfHeight = ctx.screenHeight / 2;
        if (m_halfWidth < 1) m_halfWidth = 1;
        if (m_halfHeight < 1) m_halfHeight = 1;

        CreateTextures(ctx);

        CORE_INFO("RTAOPass initialized ({}x{} raw, {}x{} output)",
                  m_halfWidth, m_halfHeight, ctx.screenWidth, ctx.screenHeight);
    }

    void Execute(RenderContext& ctx) override {
        if (!m_traceShader || !m_traceShader->isValid()) return;
        if (!m_denoiseShader || !m_denoiseShader->isValid()) return;
        if (!ctx.gpuResources) return;

        auto* bvh = ctx.scene ? ctx.scene->GetAccelerationStructure() : nullptr;
        if (bvh && bvh->IsBuilt()) {
            bvh->Bind();
        }

        // --- Step 1: Trace RTAO rays ---
        m_traceShader->use();

        // Bind output image
        glBindImageTexture(0, m_rawAOTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);

        // Bind G-Buffer inputs
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ctx.gDepth);
        m_traceShader->setInt("u_Depth", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, ctx.gNormalRoughness);
        m_traceShader->setInt("u_Normals", 1);

        // Bind blue noise
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, ctx.gpuResources->blueNoise.GetVec2NoiseTexture());
        m_traceShader->setInt("u_BlueNoise", 2);

        // Set uniforms
        m_traceShader->setMat4("u_InverseView", glm::inverse(ctx.view));
        m_traceShader->setMat4("u_InverseProjection", glm::inverse(ctx.unjitteredProj));
        m_traceShader->setFloat("u_AORadius", ctx.rtao.radius);
        m_traceShader->setFloat("u_AOIntensity", ctx.rtao.intensity);
        m_traceShader->setInt("u_FrameCounter", m_frameCounter++);
        m_traceShader->setVec2("u_ScreenSize",
            static_cast<float>(ctx.screenWidth),
            static_cast<float>(ctx.screenHeight));

        // Dispatch at half resolution
        int groupsX = (m_halfWidth  + 7) / 8;
        int groupsY = (m_halfHeight + 7) / 8;
        m_traceShader->dispatch(groupsX, groupsY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        if (bvh && bvh->IsBuilt()) {
            bvh->Unbind();
        }
        // --- Step 2: Temporal accumulation ---
        // (For now, we skip explicit temporal accumulation and rely on
        //  the denoise pass. Full temporal accumulation can be added
        //  via the TemporalFramework as a follow-up.)

        // --- Step 3: Bilateral denoise ---
        m_denoiseShader->use();

        glBindImageTexture(0, m_rawAOTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16F);
        glBindImageTexture(1, ctx.gpuResources->aoTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ctx.gDepth);
        m_denoiseShader->setInt("u_Depth", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, ctx.gNormalRoughness);
        m_denoiseShader->setInt("u_Normals", 1);

        m_denoiseShader->setVec2("u_TexelSize",
            1.0f / static_cast<float>(ctx.screenWidth),
            1.0f / static_cast<float>(ctx.screenHeight));

        int denoiseGroupsX = (ctx.screenWidth  + 7) / 8;
        int denoiseGroupsY = (ctx.screenHeight + 7) / 8;
        m_denoiseShader->dispatch(denoiseGroupsX, denoiseGroupsY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    void Resize(RenderContext& ctx, int width, int height) override {
        m_halfWidth  = width / 2;
        m_halfHeight = height / 2;
        if (m_halfWidth < 1) m_halfWidth = 1;
        if (m_halfHeight < 1) m_halfHeight = 1;

        DestroyTextures();
        CreateTextures(ctx);
    }

    void Shutdown() override {
        DestroyTextures();
        delete m_traceShader;
        delete m_denoiseShader;
        m_traceShader = nullptr;
        m_denoiseShader = nullptr;
    }

    const char* GetName() const override { return "RTAOPass"; }

    GLuint GetAOTexture() const override {
        return m_rawAOTexture; // Note: final output goes to ctx.gpuResources->aoTexture
    }

private:
    Pipeline* m_traceShader   = nullptr;
    Pipeline* m_denoiseShader = nullptr;
    GLuint    m_rawAOTexture  = 0;  // Half-res R16F intermediate
    int       m_halfWidth     = 0;
    int       m_halfHeight    = 0;
    int       m_frameCounter  = 0;

    void CreateTextures(RenderContext& /*ctx*/) {
        // Raw AO (half-res, R16F for precision during temporal accumulation)
        glGenTextures(1, &m_rawAOTexture);
        glBindTexture(GL_TEXTURE_2D, m_rawAOTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, m_halfWidth, m_halfHeight, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void DestroyTextures() {
        if (m_rawAOTexture) { glDeleteTextures(1, &m_rawAOTexture); m_rawAOTexture = 0; }
    }
};

} // namespace lgt
