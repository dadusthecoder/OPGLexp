#pragma once
#include "RenderPass.h"

namespace lgt {

// ============================================================
// ToneMapPass — ACES filmic tone mapping + gamma correction
// Reads: ctx.hdrColorTexture (or ctx.bloomTexture if bloom is active)
// Writes: to the currently bound FBO (the viewport framebuffer)
// ============================================================
class ToneMapPass : public RenderPass {
public:
    ToneMapPass() = default;
    ~ToneMapPass() override = default;

    void Init(RenderContext& ctx) override {
        m_shader = new Pipeline("res/shaders/ToneMap.shader");
        CreateFullscreenQuad(ctx);
    }

    void Execute(RenderContext& ctx) override {
        if (!m_shader || !m_shader->isValid()) return;

        // We're writing to the currently bound FBO (the viewport FBO)
        // Disable depth test — we're just drawing a fullscreen quad
        glDisable(GL_DEPTH_TEST);

        m_shader->use();

        // Bind HDR buffer
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ctx.hdrTexture);
        m_shader->setInt("u_HDRBuffer", 0);

        // Bind bloom texture if available
        if (ctx.bloom.enabled && ctx.bloomTexture != 0) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, ctx.bloomTexture);
            m_shader->setInt("u_BloomTexture", 1);
            m_shader->setBool("u_BloomEnabled", true);
            m_shader->setFloat("u_BloomStrength", ctx.bloom.strength);
        } else {
            m_shader->setBool("u_BloomEnabled", false);
        }

        // Set tone mapping parameters
        m_shader->setFloat("u_Exposure", ctx.hdr.exposure);
        m_shader->setFloat("u_Gamma", ctx.hdr.gamma);
        m_shader->setFloat("u_WhitePoint", ctx.hdr.whitePoint);

        DrawFullscreenQuad(ctx);

        glEnable(GL_DEPTH_TEST);
    }

    void Shutdown() override {
        delete m_shader;
        m_shader = nullptr;
    }

    const char* GetName() const override { return "ToneMapPass"; }

private:
    Pipeline* m_shader = nullptr;
};

} // namespace lgt
