#pragma once
#include "RenderPass.h"
#include "Shader.h"
#include "GPUResources.h"
#include "DDGIVolume.h"
#include "Scene.h"

namespace lgt {

class DeferredLightingPass : public RenderPass {
public:
    DeferredLightingPass() = default;
    ~DeferredLightingPass() override = default;

    void Init(RenderContext& ctx) override {
        m_shader = new Pipeline("res/shaders/DeferredLighting.shader");
        CreateFullscreenQuad(ctx);
    }

    void Execute(RenderContext& ctx) override {
        if (!m_shader || !m_shader->isValid()) return;

        glBindFramebuffer(GL_FRAMEBUFFER, ctx.hdrFBO);
        glViewport(0, 0, ctx.screenWidth, ctx.screenHeight);

        // Clear HDR buffer
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT); // No depth buffer on hdrFBO anymore! Depth is in G-Buffer.

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        m_shader->use();

        // Bind G-Buffer
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, ctx.gAlbedoMetallic); m_shader->setInt("u_gAlbedoMetallic", 0);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, ctx.gNormalRoughness); m_shader->setInt("u_gNormalRoughness", 1);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, ctx.gEmissive); m_shader->setInt("u_gEmissive", 2);
        glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, ctx.gDepth); m_shader->setInt("u_gDepth", 3);

        // Bind IBL
        glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.irradianceMap); m_shader->setInt("u_IrradianceMap", 4);
        glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.prefilterMap); m_shader->setInt("u_PrefilterMap", 5);
        glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, ctx.brdfLUT); m_shader->setInt("u_BrdfLUT", 6);
        m_shader->setBool("u_HasIBL", ctx.hasIBL);

        // Bind AO (can be SSAO or RTAO)
        glActiveTexture(GL_TEXTURE7); 
        if (ctx.aoMode == RenderContext::AOMode::RTAO && ctx.gpuResources) {
            glBindTexture(GL_TEXTURE_2D, ctx.gpuResources->aoTexture);
        } else {
            glBindTexture(GL_TEXTURE_2D, ctx.aoTexture);
        }
        m_shader->setInt("u_AOTexture", 7);
        m_shader->setBool("u_SSAOEnabled", ctx.aoMode != RenderContext::AOMode::OFF);

        // Bind DDGI
        m_shader->setBool("u_DDGIEnabled", ctx.ddgi.enabled);
        if (ctx.ddgi.enabled && ctx.scene && !ctx.scene->GetProbeVolumes().empty()) {
            auto& volume = ctx.scene->GetProbeVolumes()[0];
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_2D, volume->irradianceAtlas);
            m_shader->setInt("u_DDGIIrradiance", 10);
            
            m_shader->setVec3("u_DDGIOrigin", volume->origin);
            m_shader->setVec3("u_DDGISpacing", volume->spacing);
            m_shader->setIVec3("u_DDGIProbeCount", volume->probeCount);
            m_shader->setFloat("u_DDGIIntensity", ctx.ddgi.giIntensity);
            m_shader->setFloat("u_DDGIBlend", ctx.ddgi.giBlend);
        }

        // Bind ShadowMap
        glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D_ARRAY, ctx.csmTextureArray); m_shader->setInt("u_ShadowMap", 8);
        m_shader->setInt("u_CascadeCount", ctx.cascadeCount);
        for (int i = 0; i < ctx.cascadeCount; ++i) {
            m_shader->setFloat("u_CascadePlaneDistances[" + std::to_string(i) + "]", ctx.cascadePlaneDistances[i]);
            m_shader->setMat4("u_LightSpaceMatrices[" + std::to_string(i) + "]", ctx.lightSpaceMatrices[i]);
        }

        // Camera Uniforms
        m_shader->setVec3("u_CameraPos", ctx.cameraPos);
        m_shader->setVec2("u_ScreenSize", static_cast<float>(ctx.screenWidth), static_cast<float>(ctx.screenHeight));
        m_shader->setMat4("u_InverseView", glm::inverse(ctx.view));
        // IMPORTANT: use the UNJITTERED projection inverse for depth reconstruction.
        // The jittered projection shifts clip-space coordinates every frame; its inverse
        // would reconstruct a subtly wrong world-space position, causing the shadow
        // sample coordinate to drift and producing camera-dependent light leaking.
        m_shader->setMat4("u_InverseProjection", glm::inverse(ctx.unjitteredProj));
        m_shader->setMat4("u_View", ctx.view);

        // Debug mode (TODO: pass actual debug mode from Renderer)
        m_shader->setInt("u_DebugMode", 0);
        m_shader->setInt("u_ShadowDebugMode", ctx.shadowDebugMode);
        m_shader->setInt("u_DDGIDebugMode", ctx.ddgiDebugMode);

        DrawFullscreenQuad(ctx);
        glEnable(GL_DEPTH_TEST);
    }

    void Shutdown() override {
        delete m_shader;
        m_shader = nullptr;
    }

    const char* GetName() const override { return "DeferredLightingPass"; }

private:
    Pipeline* m_shader = nullptr;
};

} // namespace lgt
