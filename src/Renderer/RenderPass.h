#pragma once
#include "Vendor/glad.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "Helpers/Logger.h"

namespace lgt {

enum class DebugMode {
    FINAL_COLOR = 0,  // Default: full PBR lighting
    BASE_COLOR_TEXTURE = 1,
    NORMAL_TEXTURE = 2,
    EMMISIVE_TEXTURE = 3,
    NORMALS_MAPPED = 4,
};
class Camera;
class Scene;
class Pipeline;

// ============================================================
// RenderContext — shared state passed to every RenderPass
// ============================================================
struct RenderContext {
    // Camera
    Camera*   camera             = nullptr;
    glm::mat4 view               = glm::mat4(1.0f);
    glm::mat4 projection         = glm::mat4(1.0f); // Jittered (for geometry pass / TAA)
    glm::mat4 unjitteredProj     = glm::mat4(1.0f); // Clean (for depth reconstruction, shadows)
    glm::mat4 prevView           = glm::mat4(1.0f); // For Velocity/TAA
    glm::mat4 prevProjection     = glm::mat4(1.0f); // For Velocity/TAA
    glm::vec2 taaJitter          = glm::vec2(0.0f); // Current frame jitter
    glm::vec3 cameraPos          = glm::vec3(0.0f);
    float     nearPlane      = 0.1f;
    float     farPlane       = 100.0f;

    // Viewport
    int screenWidth  = 1920;
    int screenHeight = 1080;

    // Scene
    Scene* scene = nullptr;

    // --- Shared GPU resources (written by one pass, read by others) ---

    // G-Buffer
    GLuint gBufferFBO         = 0;
    GLuint gAlbedoMetallic    = 0; // GL_RGBA8
    GLuint gNormalRoughness   = 0; // GL_RGBA16F
    GLuint gEmissive          = 0; // GL_RGBA16F (can include AO/material flags)
    GLuint gVelocity          = 0; // GL_RG16F
    GLuint gDepth             = 0; // GL_DEPTH24_STENCIL8
    int    gBufferDebugView   = 0; // 0: Final, 1: Albedo, 2: Normal, 3: Emissive, 4: Velocity, 5: Depth, 6: SSAO

    // HDR Color Buffer (FP16) - Used for lighting output and post-processing
    GLuint hdrFBO          = 0;
    GLuint hdrTexture      = 0;  // GL_RGBA16F
    GLuint hdrDepthRBO     = 0;  // NOT USED ANYMORE (Replaced by gDepth)

    // TAA History
    GLuint taaHistoryFBOs[2]     = {0, 0};
    GLuint taaHistoryTextures[2] = {0, 0}; // GL_RGBA16F
    int    taaFrameIndex         = 0;      // 0 or 1
    int    taaJitterIndex        = 0;      // 0 to 15

    // SSAO
    GLuint aoTexture       = 0;  // GL_R8

    // Bloom
    GLuint bloomTexture    = 0;  // Combined bloom result

    // IBL
    GLuint irradianceMap   = 0;
    GLuint prefilterMap    = 0;
    GLuint brdfLUT         = 0;
    GLuint envCubemap      = 0;
    bool   hasIBL          = false;

    // CSM Shadows
    GLuint csmTextureArray       = 0;
    int    cascadeCount          = 0;
    float  cascadePlaneDistances[4] = {};
    glm::mat4 lightSpaceMatrices[4] = {};

    // Skybox Settings
    struct {
        float lod = 0.0f;
    } skybox;

    // Fullscreen Quad VAO
    GLuint quadVAO = 0;
    GLuint quadVBO = 0;

    // Forward+ Resources
    GLuint lightsSSBO              = 0;
    GLuint visibleLightIndicesSSBO = 0;
    int    workGroupsX = 0;
    int    workGroupsY = 0;

    // --- Settings ---
    struct HDRSettings {
        float exposure   = 1.0f;
        float whitePoint = 1.0f;
        float gamma      = 2.2f;
    } hdr;

    struct TAASettings {
        bool  enabled  = true;
        float feedback = 0.9f; // How much of previous frame to keep
    } taa;

    struct BloomSettings {
        bool  enabled   = true;
        float threshold = 1.0f;
        float softKnee  = 0.5f;
        float strength  = 0.5f;
        float filterRadius = 0.005f;
    } bloom;

    struct SSAOSettings {
        bool  enabled   = true;
        float radius    = 0.5f;
        float bias      = 0.025f;
        float intensity = 1.0f;
        int   samples   = 32;
    } ssao;

    // --- Ambient Occlusion Mode ---
    enum class AOMode { OFF, SSAO, RTAO };
    AOMode aoMode = AOMode::SSAO;

    struct RTAOSettings {
        float radius        = 2.0f;    // World-space AO ray max distance
        float intensity     = 1.5f;    // AO strength multiplier
        float temporalBlend = 0.9f;    // History weight (higher = smoother, more ghosting)
    } rtao;

    // --- DDGI Settings ---
    struct DDGISettings {
        bool      enabled       = false;
        float     giIntensity   = 1.0f;    // GI strength multiplier
        float     giBlend       = 0.8f;    // 0 = pure IBL diffuse, 1 = pure DDGI
        float     hysteresis    = 0.97f;   // Temporal blend for probe updates
        float     maxRayDist    = 50.0f;   // Max probe ray distance
        float     normalBias    = 0.25f;   // Bias along normal to prevent self-shadowing
        int       qualityPreset = 2;       // 0=Low, 1=Medium, 2=High, 3=Ultra
        bool      showProbes    = false;   // Debug: visualize probe positions
    } ddgi;

    struct EnvironmentSettings {
        float rotation  = 0.0f;
        float intensity = 1.0f;
        bool  showSkybox = true;
    } environment;

    // --- Persistent GPU Resources (owned by Renderer, not per-frame) ---
    struct GPUResources* gpuResources = nullptr;

    // Debug
    DebugMode debugMode = DebugMode::FINAL_COLOR;
    int shadowDebugMode = 0; // 0=off, 1-10=instrumentation modes
    int ddgiDebugMode = 0; // 0=off, 1-10=instrumentation modes
    bool visualizeTiles = false;
};

// ============================================================
// RenderPass — abstract base class for all render passes
// ============================================================
class RenderPass {
public:
    virtual ~RenderPass() = default;

    /// Called once during renderer initialization
    virtual void Init(RenderContext& ctx) = 0;

    /// Called every frame
    virtual void Execute(RenderContext& ctx) = 0;

    /// Called when the viewport is resized
    virtual void Resize(RenderContext& ctx, int width, int height) {}

    /// Called during shutdown to clean up GPU resources
    virtual void Shutdown() {}

    /// Human-readable name for profiling / debug
    virtual const char* GetName() const = 0;
};

// ============================================================
// RenderGraph — manages the ordered list of passes
// ============================================================
class RenderGraph {
public:
    RenderGraph() = default;
    ~RenderGraph() = default;

    // Non-copyable
    RenderGraph(const RenderGraph&)            = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    
    RenderGraph(RenderGraph&&)            = default;
    RenderGraph& operator=(RenderGraph&&) = default;

    /// Add a pass to the end of the pipeline
    template <typename T, typename... Args>
    T* AddPass(Args&&... args) {
        auto pass = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = pass.get();
        m_passes.push_back(std::move(pass));
        return ptr;
    }

    /// Initialize all passes
    void Init(RenderContext& ctx) {
        for (auto& pass : m_passes) {
            pass->Init(ctx);
        }
    }

    /// Execute all passes in order
    void Execute(RenderContext& ctx) {
        for (auto& pass : m_passes) {
            pass->Execute(ctx);
        }
    }

    /// Resize all passes
    void Resize(RenderContext& ctx, int width, int height) {
        for (auto& pass : m_passes) {
            pass->Resize(ctx, width, height);
        }
    }

    /// Shutdown all passes
    void Shutdown() {
        for (auto& pass : m_passes) {
            pass->Shutdown();
        }
        m_passes.clear();
    }

    /// Get a pass by type (for debug/UI)
    template <typename T>
    T* GetPass() const {
        for (auto& pass : m_passes) {
            T* result = dynamic_cast<T*>(pass.get());
            if (result) return result;
        }
        return nullptr;
    }

private:
    std::vector<std::unique_ptr<RenderPass>> m_passes;
};

// ============================================================
// Utility: Create/resize G-Buffer and HDR framebuffers
// ============================================================
inline void CreateGBuffer(RenderContext& ctx, int width, int height) {
    // Clean up old G-Buffer
    if (ctx.gBufferFBO != 0) {
        glDeleteFramebuffers(1, &ctx.gBufferFBO); ctx.gBufferFBO = 0;
        glDeleteTextures(1, &ctx.gAlbedoMetallic); ctx.gAlbedoMetallic = 0;
        glDeleteTextures(1, &ctx.gNormalRoughness); ctx.gNormalRoughness = 0;
        glDeleteTextures(1, &ctx.gEmissive); ctx.gEmissive = 0;
        glDeleteTextures(1, &ctx.gVelocity); ctx.gVelocity = 0;
        glDeleteTextures(1, &ctx.gDepth); ctx.gDepth = 0;
    }

    glGenFramebuffers(1, &ctx.gBufferFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ctx.gBufferFBO);

    // 1. Albedo + Metallic (RGBA8 is usually enough, but RGBA16F is safer for now if we want to avoid precision issues)
    glGenTextures(1, &ctx.gAlbedoMetallic);
    glBindTexture(GL_TEXTURE_2D, ctx.gAlbedoMetallic);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctx.gAlbedoMetallic, 0);

    // 2. Normal + Roughness (RGBA16F required for precision)
    glGenTextures(1, &ctx.gNormalRoughness);
    glBindTexture(GL_TEXTURE_2D, ctx.gNormalRoughness);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, ctx.gNormalRoughness, 0);

    // 3. Emissive (RGBA16F for HDR values)
    glGenTextures(1, &ctx.gEmissive);
    glBindTexture(GL_TEXTURE_2D, ctx.gEmissive);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, ctx.gEmissive, 0);

    // 4. Velocity (RG16F)
    glGenTextures(1, &ctx.gVelocity);
    glBindTexture(GL_TEXTURE_2D, ctx.gVelocity);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, ctx.gVelocity, 0);

    // Tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
    unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(4, attachments);

    // Depth buffer (Texture instead of RBO so we can sample it for SSAO/SSR)
    glGenTextures(1, &ctx.gDepth);
    glBindTexture(GL_TEXTURE_2D, ctx.gDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, ctx.gDepth, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        CORE_ERROR("G-Buffer not complete!");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Also resize TAA buffers
    for (int i = 0; i < 2; ++i) {
        if (ctx.taaHistoryFBOs[i] != 0) {
            glDeleteFramebuffers(1, &ctx.taaHistoryFBOs[i]); ctx.taaHistoryFBOs[i] = 0;
            glDeleteTextures(1, &ctx.taaHistoryTextures[i]); ctx.taaHistoryTextures[i] = 0;
        }
        glGenFramebuffers(1, &ctx.taaHistoryFBOs[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, ctx.taaHistoryFBOs[i]);
        glGenTextures(1, &ctx.taaHistoryTextures[i]);
        glBindTexture(GL_TEXTURE_2D, ctx.taaHistoryTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctx.taaHistoryTextures[i], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

inline void CreateHDRFramebuffer(RenderContext& ctx, int width, int height) {
    if (ctx.hdrFBO != 0) {
        glDeleteFramebuffers(1, &ctx.hdrFBO); ctx.hdrFBO = 0;
        glDeleteTextures(1, &ctx.hdrTexture); ctx.hdrTexture = 0;
    }
    
    glGenFramebuffers(1, &ctx.hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ctx.hdrFBO);

    glGenTextures(1, &ctx.hdrTexture);
    glBindTexture(GL_TEXTURE_2D, ctx.hdrTexture);
    // Use GL_RGBA16F for high dynamic range
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctx.hdrTexture, 0);
    if (ctx.gDepth != 0) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, ctx.gDepth, 0);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        CORE_ERROR("HDR Framebuffer not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/// Initialize the fullscreen quad VAO (for post-processing)
inline void CreateFullscreenQuad(RenderContext& ctx) {
    if (ctx.quadVAO != 0) return;

    float quadVertices[] = {
        // positions   // texcoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &ctx.quadVAO);
    glGenBuffers(1, &ctx.quadVBO);
    glBindVertexArray(ctx.quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, ctx.quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

/// Draw a fullscreen quad (using the gl_VertexID trick to draw a single large triangle)
inline void DrawFullscreenQuad(const RenderContext& ctx) {
    // We bind an empty VAO (or any VAO) to satisfy Core Profile requirements,
    // but the shaders use gl_VertexID to generate a single triangle (3 vertices)
    // that covers the entire screen.
    glBindVertexArray(ctx.quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

} // namespace lgt
