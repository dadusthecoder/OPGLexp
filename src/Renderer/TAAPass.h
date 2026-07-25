#pragma once
#include "RenderPass.h"

namespace lgt {

class TAAPass : public RenderPass {
public:
    TAAPass() = default;
    ~TAAPass() override = default;

    void Init(RenderContext& ctx) override {
        m_taaShader = new Pipeline("res/shaders/TAA.shader");
        CreateFullscreenQuad(ctx);
    }

    void Execute(RenderContext& ctx) override {
        if (!ctx.taa.enabled || !m_taaShader || !m_taaShader->isValid()) return;

        int currIndex = ctx.taaFrameIndex;
        int prevIndex = 1 - currIndex;

        // Bind the current history FBO to write the resolved image
        glBindFramebuffer(GL_FRAMEBUFFER, ctx.taaHistoryFBOs[currIndex]);
        glViewport(0, 0, ctx.screenWidth, ctx.screenHeight);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        m_taaShader->use();

        // Bind current HDR frame (what was just rendered)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ctx.hdrTexture);
        m_taaShader->setInt("u_CurrentFrame", 0);

        // Bind history frame
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, ctx.taaHistoryTextures[prevIndex]);
        m_taaShader->setInt("u_HistoryFrame", 1);

        // Bind velocity buffer
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, ctx.gVelocity);
        m_taaShader->setInt("u_VelocityTexture", 2);

        // Bind depth buffer for dilation
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, ctx.gDepth);
        m_taaShader->setInt("u_DepthTexture", 3);

        m_taaShader->setFloat("u_Feedback", ctx.taa.feedback);

        // Draw fullscreen quad
        DrawFullscreenQuad(ctx);

        // Now that the resolved image is in taaHistoryTextures[currIndex],
        // we must copy it back to the HDR buffer so that subsequent passes (Bloom, ToneMap) can read it.
        glBindFramebuffer(GL_READ_FRAMEBUFFER, ctx.taaHistoryFBOs[currIndex]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ctx.hdrFBO);
        glBlitFramebuffer(0, 0, ctx.screenWidth, ctx.screenHeight, 0, 0, ctx.screenWidth, ctx.screenHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // Restore context
        glEnable(GL_DEPTH_TEST);
    }

    void Shutdown() override {
        delete m_taaShader;
        m_taaShader = nullptr;
    }

    const char* GetName() const override { return "TAAPass"; }

private:
    Pipeline* m_taaShader = nullptr;
};

} // namespace lgt
