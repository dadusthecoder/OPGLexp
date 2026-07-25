#pragma once
#include "RenderPass.h"
#include <algorithm>

namespace lgt {

// ============================================================
// BloomPass — Downsample/Upsample pyramid bloom
//
// Pipeline:
//   HDR Buffer → Threshold Extract → Downsample (1/2 → 1/4 → ... → 1/N)
//                                  → Upsample   (1/N → ... → 1/4 → 1/2)
//                                  → Result in ctx.bloomTexture
// ============================================================
class BloomPass : public RenderPass {
public:
    static constexpr int MAX_MIP_LEVELS = 8;

    BloomPass() = default;
    ~BloomPass() override = default;

    void Init(RenderContext& ctx) override {
        m_extractShader    = new Pipeline("res/shaders/BloomExtract.shader");
        m_downsampleShader = new Pipeline("res/shaders/BloomDownsample.shader");
        m_upsampleShader   = new Pipeline("res/shaders/BloomUpsample.shader");

        CreateFullscreenQuad(ctx);
    }

    void Execute(RenderContext& ctx) override {
        if (!ctx.bloom.enabled) return;
        if (!m_extractShader || !m_downsampleShader || !m_upsampleShader) return;
        if (m_mipCount == 0) return;

        glDisable(GL_DEPTH_TEST);

        // --- 1. Extract bright pixels ---
        glBindFramebuffer(GL_FRAMEBUFFER, m_mipFBOs[0]);
        glViewport(0, 0, m_mipSizes[0].x, m_mipSizes[0].y);

        m_extractShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ctx.hdrTexture);
        m_extractShader->setInt("u_HDRBuffer", 0);
        m_extractShader->setFloat("u_Threshold", ctx.bloom.threshold);
        m_extractShader->setFloat("u_SoftKnee", ctx.bloom.softKnee);

        DrawFullscreenQuad(ctx);

        // --- 2. Progressive Downsample ---
        m_downsampleShader->use();
        for (int i = 1; i < m_mipCount; ++i) {
            glBindFramebuffer(GL_FRAMEBUFFER, m_mipFBOs[i]);
            glViewport(0, 0, m_mipSizes[i].x, m_mipSizes[i].y);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_mipTextures[i - 1]);
            m_downsampleShader->setInt("u_SourceTexture", 0);
            m_downsampleShader->setVec2("u_SrcResolution",
                static_cast<float>(m_mipSizes[i - 1].x),
                static_cast<float>(m_mipSizes[i - 1].y));

            DrawFullscreenQuad(ctx);
        }

        // --- 3. Progressive Upsample (with additive blending) ---
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);  // Additive
        glBlendEquation(GL_FUNC_ADD);

        m_upsampleShader->use();
        m_upsampleShader->setVec2("u_FilterRadius", ctx.bloom.filterRadius, ctx.bloom.filterRadius);

        for (int i = m_mipCount - 1; i > 0; --i) {
            glBindFramebuffer(GL_FRAMEBUFFER, m_mipFBOs[i - 1]);
            glViewport(0, 0, m_mipSizes[i - 1].x, m_mipSizes[i - 1].y);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_mipTextures[i]);
            m_upsampleShader->setInt("u_SourceTexture", 0);

            DrawFullscreenQuad(ctx);
        }

        glDisable(GL_BLEND);

        // The final bloom result is in m_mipTextures[0]
        ctx.bloomTexture = m_mipTextures[0];

        // Restore viewport
        glViewport(0, 0, ctx.screenWidth, ctx.screenHeight);
        glEnable(GL_DEPTH_TEST);
    }

    void Resize(RenderContext& ctx, int width, int height) override {
        destroyMipChain();
        createMipChain(width, height);
    }

    void Shutdown() override {
        destroyMipChain();
        delete m_extractShader;    m_extractShader = nullptr;
        delete m_downsampleShader; m_downsampleShader = nullptr;
        delete m_upsampleShader;   m_upsampleShader = nullptr;
    }

    const char* GetName() const override { return "BloomPass"; }

    int GetMipCount() const { return m_mipCount; }

private:
    Pipeline* m_extractShader    = nullptr;
    Pipeline* m_downsampleShader = nullptr;
    Pipeline* m_upsampleShader   = nullptr;

    GLuint     m_mipFBOs[MAX_MIP_LEVELS]     = {};
    GLuint     m_mipTextures[MAX_MIP_LEVELS] = {};
    glm::ivec2 m_mipSizes[MAX_MIP_LEVELS]    = {};
    int        m_mipCount = 0;

    void createMipChain(int width, int height) {
        // Start at half resolution
        int mipWidth  = width / 2;
        int mipHeight = height / 2;
        m_mipCount = 0;

        for (int i = 0; i < MAX_MIP_LEVELS; ++i) {
            if (mipWidth < 4 || mipHeight < 4) break;

            m_mipSizes[i] = glm::ivec2(mipWidth, mipHeight);

            // Create texture
            glGenTextures(1, &m_mipTextures[i]);
            glBindTexture(GL_TEXTURE_2D, m_mipTextures[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, mipWidth, mipHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Create FBO
            glGenFramebuffers(1, &m_mipFBOs[i]);
            glBindFramebuffer(GL_FRAMEBUFFER, m_mipFBOs[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_mipTextures[i], 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                CORE_ERROR("Bloom mip FBO {} not complete!", i);
            }

            m_mipCount++;
            mipWidth  /= 2;
            mipHeight /= 2;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void destroyMipChain() {
        for (int i = 0; i < m_mipCount; ++i) {
            if (m_mipFBOs[i])     { glDeleteFramebuffers(1, &m_mipFBOs[i]); m_mipFBOs[i] = 0;     m_mipFBOs[i] = 0; }
            if (m_mipTextures[i]) { glDeleteTextures(1, &m_mipTextures[i]); m_mipTextures[i] = 0; m_mipTextures[i] = 0; }
        }
        m_mipCount = 0;
    }
};

} // namespace lgt
