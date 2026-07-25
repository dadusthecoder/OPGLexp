#pragma once
#include "RenderPass.h"
#include <type_traits>
#include <random>

namespace lgt {

// ============================================================
// AmbientOcclusionPass — abstract interface for AO passes
// Designed to be swappable: SSAO → GTAO → HBAO
// ============================================================
class AmbientOcclusionPass : public RenderPass {
public:
    virtual ~AmbientOcclusionPass() = default;
    virtual GLuint GetAOTexture() const = 0;
};

// ============================================================
// SSAOPass — Screen-Space Ambient Occlusion
// Reads: ctx.depthMapTexture
// Writes: ctx.aoTexture
// ============================================================
class SSAOPass : public AmbientOcclusionPass {
public:
    SSAOPass() = default;
    ~SSAOPass() override = default;

    void Init(RenderContext& ctx) override {
        m_ssaoShader = new Pipeline("res/shaders/SSAO.shader");
        m_blurShader = new Pipeline("res/shaders/SSAOBlur.shader");

        CreateFullscreenQuad(ctx);
        generateKernel();
        generateNoiseTexture();
    }

    void Execute(RenderContext& ctx) override {
        if (!ctx.ssao.enabled) return;
        if (!m_ssaoShader || !m_blurShader) return;
        if (m_ssaoFBO == 0) return;

        glDisable(GL_DEPTH_TEST);

        // --- 1. Generate SSAO ---
        glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoFBO);
        glViewport(0, 0, m_width, m_height);
        glClear(GL_COLOR_BUFFER_BIT);

        m_ssaoShader->use();

        // Bind depth texture from G-Buffer
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ctx.gDepth);
        m_ssaoShader->setInt("u_DepthTexture", 0);

        // Bind noise texture
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_noiseTexture);
        m_ssaoShader->setInt("u_NoiseTexture", 1);

        // Bind normal texture from G-Buffer
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, ctx.gNormalRoughness);
        m_ssaoShader->setInt("u_gNormalRoughness", 2);

        // Set uniforms
        m_ssaoShader->setMat4("u_Projection", ctx.projection);
        m_ssaoShader->setMat4("u_View", ctx.view);

        // Compute inverse projection
        glm::mat4 invProj = glm::inverse(ctx.projection);
        m_ssaoShader->setMat4("u_InvProjection", invProj);

        m_ssaoShader->setVec2("u_NoiseScale",
            static_cast<float>(m_width) / 4.0f,
            static_cast<float>(m_height) / 4.0f);

        m_ssaoShader->setFloat("u_Radius", ctx.ssao.radius);
        m_ssaoShader->setFloat("u_Bias", ctx.ssao.bias);
        m_ssaoShader->setFloat("u_Intensity", ctx.ssao.intensity);
        m_ssaoShader->setInt("u_KernelSize", ctx.ssao.samples);

        m_ssaoShader->setFloat("u_Near", ctx.nearPlane);
        m_ssaoShader->setFloat("u_Far", ctx.farPlane);

        for (int i = 0; i < KERNEL_SIZE; ++i) {
            m_ssaoShader->setVec3("u_Samples[" + std::to_string(i) + "]", m_kernel[i]);
        }

        DrawFullscreenQuad(ctx);

        // --- 2. Blur SSAO ---
        glBindFramebuffer(GL_FRAMEBUFFER, m_blurFBO);
        glViewport(0, 0, m_width, m_height);
        glClear(GL_COLOR_BUFFER_BIT);

        m_blurShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_ssaoTexture);
        m_blurShader->setInt("u_SSAOTexture", 0);

        DrawFullscreenQuad(ctx);

        // Set the blurred AO texture as output
        ctx.aoTexture = m_blurTexture;

        // Restore
        glViewport(0, 0, ctx.screenWidth, ctx.screenHeight);
        glEnable(GL_DEPTH_TEST);
    }

    void Resize(RenderContext& ctx, int width, int height) override {
        destroyFBOs();
        createFBOs(width, height);
    }

    void Shutdown() override {
        destroyFBOs();
        if (m_noiseTexture) { glDeleteTextures(1, &m_noiseTexture); m_noiseTexture = 0; }
        delete m_ssaoShader; m_ssaoShader = nullptr;
        delete m_blurShader; m_blurShader = nullptr;
    }

    const char* GetName() const override { return "SSAOPass"; }
    GLuint GetAOTexture() const override { return m_blurTexture; }

private:
    static constexpr int KERNEL_SIZE = 64;

    Pipeline* m_ssaoShader = nullptr;
    Pipeline* m_blurShader = nullptr;

    // SSAO FBO + texture
    GLuint m_ssaoFBO     = 0;
    GLuint m_ssaoTexture = 0;

    // Blur FBO + texture
    GLuint m_blurFBO     = 0;
    GLuint m_blurTexture = 0;

    // Noise texture (4x4 random tangent vectors)
    GLuint m_noiseTexture = 0;

    // Hemisphere kernel
    glm::vec3 m_kernel[KERNEL_SIZE] = {};

    int m_width = 0, m_height = 0;

    void generateKernel() {
        std::mt19937 gen(42); // Deterministic seed for reproducibility
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        for (int i = 0; i < KERNEL_SIZE; ++i) {
            glm::vec3 sample(
                dist(gen) * 2.0f - 1.0f,
                dist(gen) * 2.0f - 1.0f,
                dist(gen)  // hemisphere — z always positive
            );
            sample = glm::normalize(sample);
            sample *= dist(gen);

            // Accelerating interpolation — more samples closer to the origin
            float scale = static_cast<float>(i) / static_cast<float>(KERNEL_SIZE);
            scale = 0.1f + scale * scale * 0.9f; // lerp(0.1, 1.0, scale*scale)
            sample *= scale;

            m_kernel[i] = sample;
        }
    }

    void generateNoiseTexture() {
        std::mt19937 gen(42);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        std::vector<glm::vec3> noise(16);
        for (int i = 0; i < 16; ++i) {
            noise[i] = glm::vec3(
                dist(gen) * 2.0f - 1.0f,
                dist(gen) * 2.0f - 1.0f,
                0.0f  // Rotate around z-axis
            );
        }

        glGenTextures(1, &m_noiseTexture);
        glBindTexture(GL_TEXTURE_2D, m_noiseTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, noise.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    void createFBOs(int width, int height) {
        m_width  = width;
        m_height = height;

        // SSAO FBO
        glGenTextures(1, &m_ssaoTexture);
        glBindTexture(GL_TEXTURE_2D, m_ssaoTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glGenFramebuffers(1, &m_ssaoFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ssaoTexture, 0);

        // Blur FBO
        glGenTextures(1, &m_blurTexture);
        glBindTexture(GL_TEXTURE_2D, m_blurTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glGenFramebuffers(1, &m_blurFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_blurFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_blurTexture, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void destroyFBOs() {
        if (m_ssaoFBO)     { glDeleteFramebuffers(1, &m_ssaoFBO); m_ssaoFBO = 0;  m_ssaoFBO = 0; }
        if (m_ssaoTexture) { glDeleteTextures(1, &m_ssaoTexture); m_ssaoTexture = 0;  m_ssaoTexture = 0; }
        if (m_blurFBO)     { glDeleteFramebuffers(1, &m_blurFBO); m_blurFBO = 0;  m_blurFBO = 0; }
        if (m_blurTexture) { glDeleteTextures(1, &m_blurTexture); m_blurTexture = 0;  m_blurTexture = 0; }
    }
};

} // namespace lgt
