#include "Renderer.h"
#include "Helpers/Logger.h"
#include "UI/Editor.h"
#include "Scene.h"
#include "Camera.h"
#include "EnvironmentMap.h"
#include <chrono>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

#include "DeferredGeometryPass.h"
#include "LightCullingPass.h"
#include "DeferredLightingPass.h"
#include "TAAPass.h"
#include "ToneMapPass.h"
#include "BloomPass.h"
#include "SSAOPass.h"
#include "SkyboxPass.h"

//----------------------------------------------------------------------
namespace lgt {
Grid::Grid() {
    m_gridShader = std::make_unique<Pipeline>("res/shaders/grid.shader");

    generateGridMesh(100, 100, 20.0f); // 100x100 grid, 20 units spacing
    setupBuffers();
}

Grid::~Grid() {
    cleanup();
}

void Grid::generateGridMesh(int width, int height, float spacing) {
    m_vertices.clear();
    m_indices.clear();

    // Generate vertices
    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {
            float xPos = (x - width * 0.5f) * spacing;
            float zPos = (z - height * 0.5f) * spacing;

            // Position (y = 0 for flat grid, will be displaced in vertex shader)
            m_vertices.push_back(xPos); // x
            m_vertices.push_back(0.0f); // y
            m_vertices.push_back(zPos); // z
        }
    }

    // Generate indices for wireframe or triangles
    for (int z = 0; z < height - 1; ++z) {
        for (int x = 0; x < width - 1; ++x) {
            int topLeft     = z * width + x;
            int topRight    = topLeft + 1;
            int bottomLeft  = (z + 1) * width + x;
            int bottomRight = bottomLeft + 1;

            // Create triangles
            m_indices.push_back(topLeft);
            m_indices.push_back(bottomLeft);
            m_indices.push_back(topRight);

            m_indices.push_back(topRight);
            m_indices.push_back(bottomLeft);
            m_indices.push_back(bottomRight);
        }
    }
}

void Grid::setupBuffers() {
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    // Vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(float), m_vertices.data(), GL_STATIC_DRAW);

    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}



void Renderer::loadSkybox(const std::string& hdrPath) {
    if (!m_envMap) {
        m_envMap = std::make_unique<EnvironmentMap>();
    }
    if (m_envMap->LoadHDR(hdrPath)) {
        m_renderCtx.envCubemap = m_envMap->GetEnvironmentCubemap();
        m_renderCtx.hasIBL = true;
        CORE_INFO("Skybox and IBL maps loaded from: {}", hdrPath);
    }
}


void Grid::render(Camera& cam, float deltaTime) {
    if (!m_gridShader || !m_gridShader->isValid())
        return;

    m_time += deltaTime;

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable depth writing but keep depth testing
    glDepthMask(GL_FALSE);

    // Use RAII shader binding
    m_gridShader->use();
    // Set matrices
    glm::mat4 model = glm::mat4(1.0f);
    m_gridShader->setMat4("u_model", model);
    m_gridShader->setMat4("u_view", cam.GetViewMatrix());
    m_gridShader->setMat4("u_projection", cam.GetProjectionMatrix());

    // Set view position
    m_gridShader->setVec3("u_viewPos", cam.GetCameraPos());

    // Set animation parameters
    m_gridShader->setFloat("u_time", m_time);
    m_gridShader->setBool("u_enableAnimation", m_settings.enableAnimation);
    m_gridShader->setFloat("u_waveAmplitude", m_settings.waveAmplitude);
    m_gridShader->setFloat("u_waveFrequency", m_settings.waveFrequency);

    // Set appearance parameters
    m_gridShader->setVec3("u_baseColor", m_settings.baseColor);
    m_gridShader->setVec3("u_gradientColor", m_settings.gradientColor);
    m_gridShader->setFloat("u_fadeDistance", m_settings.fadeDistance);
    m_gridShader->setFloat("u_gridIntensity", m_settings.gridIntensity);
    m_gridShader->setBool("u_enableGrid", m_settings.enableGrid);
    m_gridShader->setBool("u_enableGradient", m_settings.enableGradient);

    // Render the mesh
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Restore render state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

GridSettings& Grid::getSetting() {
    return m_settings;
}

void Grid::cleanup() {
    if (m_VAO) {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        glDeleteBuffers(1, &m_EBO);
        m_VAO = m_VBO = m_EBO = 0;
    }
}

Renderer::Renderer(Scene* scene, Camera* camera)
    : m_frameCount(0),
      m_fps(0.0f),
      m_lastTime(0.0),
      m_currentRenderMode(RenderMode::FILL) {

    ASSERT(scene && camera);
    camera_ = camera;
    scene_  = scene;
}

Renderer::~Renderer() {}

void Renderer::init() {
    testPipeline       = new Pipeline("res/shaders/PBR.shader");
    depthPrepassShader = new Pipeline("res/shaders/DepthPrepass.shader");
    lightCullingShader = new Pipeline("res/shaders/LightCulling.comp");
    lightGizmoShader   = new Pipeline("res/shaders/LightGizmo.shader");
    selectionShader    = new Pipeline("res/shaders/Selection.shader");
    outlineShader      = new Pipeline("res/shaders/Outline.shader");

    glGenFramebuffers(1, &m_selectionFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_selectionFBO);
    glGenTextures(1, &m_selectionTexture);
    glBindTexture(GL_TEXTURE_2D, m_selectionTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, 1920, 1080, 0, GL_RED_INTEGER, GL_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_selectionTexture, 0);

    glGenTextures(1, &m_selectionDepth);
    glBindTexture(GL_TEXTURE_2D, m_selectionDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 1920, 1080, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_selectionDepth, 0);
    GLenum drawBuf = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuf);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Create reusable empty VAO for attribute-less draws (gizmos)
    glGenVertexArrays(1, &m_emptyVAO);
    
    // Load icons
    int width, height, channels;
    unsigned char* data = stbi_load("res/textures/lightbulb.jpg", &width, &height, &channels, 4);
    if (data) {
        glGenTextures(1, &m_PointLightIcon);
        glBindTexture(GL_TEXTURE_2D, m_PointLightIcon);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    data = stbi_load("res/textures/sun.jpg", &width, &height, &channels, 4);
    if (data) {
        glGenTextures(1, &m_DirLightIcon);
        glBindTexture(GL_TEXTURE_2D, m_DirLightIcon);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    
    // CSM
    shadowShader = new Pipeline("res/shaders/Shadow.shader");
    m_shadowCascadeLevels = { 45.0f, 25.0f, 50.0f }; // cascade far planes (using 45.0f just for first cascade example, wait I should use actual far planes)
    m_shadowCascadeLevels = { 10.0f, 25.0f, 50.0f };
    m_csmBuffer = std::make_unique<CascadedShadowBuffer>(2048, 2048, 4);
    
    createMaterailBuffer(g_MaterialGPU.size());
    uploadMaterialBuffer(g_MaterialGPU.data(), g_MaterialGPU.size());

    // Initially setup for standard 1920x1080, will be updated on resize
    setupForwardPlus(1920, 1080);

    // --- Deferred Pipeline & Post-Processing ---
    CreateGBuffer(m_renderCtx, 1920, 1080);
    CreateHDRFramebuffer(m_renderCtx, 1920, 1080);
    CreateFullscreenQuad(m_renderCtx);

    // Initialize render passes
    m_geometryPass = m_renderGraph.AddPass<DeferredGeometryPass>();
    m_cullingPass  = m_renderGraph.AddPass<LightCullingPass>();
    m_ssaoPass     = m_renderGraph.AddPass<SSAOPass>();
    m_lightingPass = m_renderGraph.AddPass<DeferredLightingPass>();
    m_skyboxPass   = m_renderGraph.AddPass<SkyboxPass>();
    m_taaPass      = m_renderGraph.AddPass<TAAPass>();
    m_bloomPass    = m_renderGraph.AddPass<BloomPass>();
    m_toneMapPass  = m_renderGraph.AddPass<ToneMapPass>();

    m_renderGraph.Init(m_renderCtx);
    m_renderGraph.Resize(m_renderCtx, 1920, 1080);

    CORE_INFO("Deferred Pipeline initialized (G-Buffer + PBR + SSAO + Bloom + ToneMap)");
}

void Renderer::shutdown() {
    // Shutdown post-processing pipeline
    m_renderGraph.Shutdown();

    // Clean up HDR FBO
    if (m_renderCtx.hdrFBO != 0) {
        glDeleteFramebuffers(1, &m_renderCtx.hdrFBO);
        glDeleteTextures(1, &m_renderCtx.hdrTexture);
        glDeleteRenderbuffers(1, &m_renderCtx.hdrDepthRBO);
    }
    if (m_renderCtx.quadVAO != 0) {
        glDeleteVertexArrays(1, &m_renderCtx.quadVAO);
        glDeleteBuffers(1, &m_renderCtx.quadVBO);
    }

    delete testPipeline;
    delete depthPrepassShader;
    delete lightCullingShader;
    delete lightGizmoShader;
    delete selectionShader;
    delete outlineShader;
    delete shadowShader;

    if (m_lightsSSBO != 0) glDeleteBuffers(1, &m_lightsSSBO);
    if (m_visibleLightIndicesSSBO != 0) glDeleteBuffers(1, &m_visibleLightIndicesSSBO);
    if (m_depthMapFBO != 0) glDeleteFramebuffers(1, &m_depthMapFBO); m_depthMapFBO = 0;
    if (m_depthMap != 0) glDeleteTextures(1, &m_depthMap); m_depthMap = 0;
}

void Renderer::GLClearError() {
    while (glGetError() != GL_NO_ERROR)
        ;
}

bool Renderer::GLLogCall(const char* function, const char* file, int line) {
    bool hasError = false;

    while (GLenum error = glGetError()) {
        const char* errorStr = getGLErrorString(error);
        CORE_CRITICAL("OPEGL : {} ", errorStr);
        hasError = true;
    }

    return !hasError;
}

const char* Renderer::getGLErrorString(GLenum error) {
    switch (error) {
    case GL_NO_ERROR:
        return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    default:
        return "Unknown GL Error";
    }
}

void Renderer::Clear(const glm::vec3& backgroundColor) const {
    GlCall(glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0f));
    GlCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

float Renderer::updateAndLogFPS(GLFWwindow* window) {
    const double currentTime = glfwGetTime();
    const double deltaTime   = currentTime - m_lastTime;

    ++m_frameCount;

    if (deltaTime >= 0.016) {
        m_fps        = static_cast<float>(m_frameCount / deltaTime);
        m_lastTime   = currentTime;
        m_frameCount = 0;
    }

    return m_fps;
}

void Renderer::setRenderMode(RenderMode mode) {
    if (m_currentRenderMode == mode) {
        return; // Avoid unnecessary state changes
    }

    m_currentRenderMode = mode;

    switch (mode) {
    case RenderMode::FILL:
        GlCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
        break;
    case RenderMode::WIREFRAME:
        GlCall(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
        break;
    case RenderMode::POINT:
        GlCall(glPolygonMode(GL_FRONT_AND_BACK, GL_POINT));
        GlCall(glPointSize(1.0f));
        break;
    }
}

void Renderer::enableDepthTesting(bool enable) {
    if (enable) {
        GlCall(glEnable(GL_DEPTH_TEST));
    } else {
        GlCall(glDisable(GL_DEPTH_TEST));
    }
}

void Renderer::enableBlending(bool enable) {
    if (enable) {
        GlCall(glEnable(GL_BLEND));
        GlCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    } else {
        GlCall(glDisable(GL_BLEND));
    }
}

void Renderer::setViewport(int width, int height) {
    if (width > 0 && height > 0) {
        if (width != m_viewportWidth || height != m_viewportHeight) {
            setupForwardPlus(width, height);

            // Resize Deferred pipeline
            CreateGBuffer(m_renderCtx, width, height);
            CreateHDRFramebuffer(m_renderCtx, width, height);
            m_renderGraph.Resize(m_renderCtx, width, height);
        }
        GlCall(glViewport(0, 0, width, height));
    }
}

void Renderer::setCamera(Camera* camera) {
    if (!camera)
        return;
    camera_ = camera;
}

void Renderer::setScene(Scene* Scene) {
    if (!Scene)
        return;
    scene_ = Scene;
}

void Renderer::setDebugMode(DebugMode mode) {
    ASSERT(testPipeline);
    m_debugMode = mode;
    testPipeline->use();
    testPipeline->setInt("u_DebugMode", (int)mode);
}

void Renderer::createMaterailBuffer(size_t size) {
    if (size == 0) size = 1; // Prevent GL_INVALID_VALUE on empty scenes
    
    if (g_MaterialSSBO != 0) {
        glDeleteBuffers(1, &g_MaterialSSBO);
        g_MaterialSSBO = 0;
    }
    
    glCreateBuffers(1, &g_MaterialSSBO);
    glNamedBufferStorage(g_MaterialSSBO, size * sizeof(MaterialGPU), nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, g_MaterialSSBO);
}

void Renderer::uploadMaterialBuffer(MaterialGPU* data, size_t size) {
    glNamedBufferSubData(g_MaterialSSBO, 0, size * sizeof(MaterialGPU), data);
}

void Renderer::updateMaterial(const MaterialGPU& mat, uint32_t index) {
    glNamedBufferSubData(g_MaterialSSBO, index * sizeof(MaterialGPU), sizeof(MaterialGPU), &mat);
}

void Renderer::updateDirtyRange(MaterialGPU* data, size_t offset, size_t count) {
    glNamedBufferSubData(g_MaterialSSBO, offset * sizeof(MaterialGPU), sizeof(MaterialGPU), data);
}

void Renderer::Initauad() {
    if (quadVAO != 0)
        return;

    float quadVertices[] = {// positions   // texcoords
                            -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

                            -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f};

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

void Renderer::renderQuad() {
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

//-----------------------FRAMEBUFFER--------------------------

RenderId FrameBuffer::GetTextureId() {
    return m_textureId;
}

RenderId FrameBuffer::GetHeight() {
    return static_cast<RenderId>(m_height);
}

RenderId FrameBuffer::GetWidth() {
    return static_cast<RenderId>(m_width);
}
void FrameBuffer::Resize(int w, int h) {
    m_width  = w;
    m_height = h;
    GlCall(glBindTexture(GL_TEXTURE_2D, m_textureId));
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    GlCall(glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffer));
    GlCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height));
}
//
void FrameBuffer::Use() {
    GlCall(glViewport(0, 0, m_width, m_height));
    GlCall(glBindFramebuffer(GL_FRAMEBUFFER, m_FBO));
}

void FrameBuffer::Unuse() {
    GlCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

FrameBuffer::FrameBuffer(int w, int h)
    : m_width(w),
      m_height(h) {
    GlCall(glGenFramebuffers(1, &m_FBO));
    GlCall(glBindFramebuffer(GL_FRAMEBUFFER, m_FBO));

    GlCall(glGenTextures(1, &m_textureId));
    GlCall(glBindTexture(GL_TEXTURE_2D, m_textureId));
    GlCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
    GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    GlCall(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_textureId, 0));

    GlCall(glGenRenderbuffers(1, &m_renderbuffer));
    GlCall(glBindRenderbuffer(GL_RENDERBUFFER, m_renderbuffer));

    GlCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height));
    GlCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_renderbuffer));

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        CORE_ERROR("FRAMEBUFFER Not complete");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FrameBuffer::~FrameBuffer() {
    glDeleteFramebuffers(1, &m_FBO);
    glDeleteRenderbuffers(1, &m_renderbuffer);
    glDeleteTextures(1, &m_textureId);
}

//-------------------------------------------------------

DepthBuffer::DepthBuffer() {
    GlCall(glGenFramebuffers(1, &m_BufferId));
    GlCall(glBindFramebuffer(GL_FRAMEBUFFER, m_BufferId));

    // Generate depth texture
    GlCall(glGenTextures(1, &m_textureId));
    GlCall(glBindTexture(GL_TEXTURE_2D, m_textureId));
    GlCall(glTexImage2D(GL_TEXTURE_2D,
                        0,
                        GL_DEPTH_COMPONENT16,
                        (GLsizei)SHADOW_WIDTH,
                        (GLsizei)SHADOW_HEIGHT,
                        0,
                        GL_DEPTH_COMPONENT,
                        GL_FLOAT,
                        nullptr));

    GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
    GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    GlCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_textureId, 0));

    GlCall(glDrawBuffer(GL_NONE));
    GlCall(glReadBuffer(GL_NONE));

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: Depth framebuffer not complete!" << std::endl;
    }

    GlCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

DepthBuffer::~DepthBuffer() {
    GlCall(glDeleteFramebuffers(1, &m_BufferId));
    GlCall(glDeleteTextures(1, &m_textureId));
}

//-------------------------------------------------------

CascadedShadowBuffer::CascadedShadowBuffer(int width, int height, int cascadeCount)
    : m_width(width), m_height(height) {
    GlCall(glGenFramebuffers(1, &m_BufferId));
    printf("Created CSM Buffer FBO: %d\n", (int)m_BufferId);

    GlCall(glGenTextures(1, &m_textureId));
    GlCall(glBindTexture(GL_TEXTURE_2D_ARRAY, m_textureId));
    GlCall(glTexImage3D(
        GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F,
        width, height, cascadeCount, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr));

    GlCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GlCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GlCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE));
    GlCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL));
    GlCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
    GlCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GlCall(glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor));

    GlCall(glBindFramebuffer(GL_FRAMEBUFFER, m_BufferId));
    GlCall(glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_textureId, 0));
    GlCall(glDrawBuffer(GL_NONE));
    GlCall(glReadBuffer(GL_NONE));

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        CORE_ERROR("ERROR: CSM Framebuffer not complete!");
    }
    GlCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

CascadedShadowBuffer::~CascadedShadowBuffer() {
    printf("Destroying CSM Buffer!\n");
    GlCall(glDeleteFramebuffers(1, &m_BufferId));
    GlCall(glDeleteTextures(1, &m_textureId));
}

void CascadedShadowBuffer::Bind() {
    GlCall(glBindFramebuffer(GL_FRAMEBUFFER, m_BufferId));
    GlCall(glViewport(0, 0, m_width, m_height));
}

void CascadedShadowBuffer::UnBind() {
    GlCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void CascadedShadowBuffer::BindForWriting(int layer) {
    GlCall(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_textureId, 0, layer));
}

void CascadedShadowBuffer::BindTex(unsigned int unit) {
    GlCall(glActiveTexture(GL_TEXTURE0 + unit));
    GlCall(glBindTexture(GL_TEXTURE_2D_ARRAY, m_textureId));
}

void CascadedShadowBuffer::UnBindTex() {
    GlCall(glBindTexture(GL_TEXTURE_2D_ARRAY, 0));
}

RenderId CascadedShadowBuffer::GetTextureId() {
    return m_textureId;
}

//-------------------------------------------------------

void DepthBuffer::Use() {
    GlCall(glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT));
    GlCall(glBindFramebuffer(GL_FRAMEBUFFER, m_BufferId));
}
void DepthBuffer::Unsue() {
    GlCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void DepthBuffer::Bind() {
    GlCall(glBindFramebuffer(GL_FRAMEBUFFER, m_BufferId));
}
void DepthBuffer::UnBind() {
    GlCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void DepthBuffer::BindTex(unsigned int slot) {
    GlCall(glActiveTexture(GL_TEXTURE0 + slot));
    GlCall(glBindTexture(GL_TEXTURE_2D, m_textureId));
}

void DepthBuffer::UnBindTex() {
    GlCall(glBindTexture(GL_TEXTURE_2D, 0));
}

RenderId DepthBuffer::GetTextureId() {
    return m_textureId;
}

void Renderer::render(class Grid* grid, float deltaTime) {
    // 0. Hot Reload Shaders (if modified)
    if (testPipeline) testPipeline->reloadIfModified();
    if (depthPrepassShader) depthPrepassShader->reloadIfModified();
    if (lightCullingShader) lightCullingShader->reloadIfModified();
    if (lightGizmoShader) lightGizmoShader->reloadIfModified();
    
    if (!scene_) {
        CORE_ERROR("No scene available");
        return;
    }
    if (!testPipeline || !depthPrepassShader || !lightCullingShader) {
        CORE_ERROR("Pipelines not available");
        return;
    }

    // Dynamically rebuild and upload the Material SSBO if new models/materials were loaded
    static size_t lastMaterialCount = 0;
    if (g_MaterialGPU.size() != lastMaterialCount) {
        lastMaterialCount = g_MaterialGPU.size();
        createMaterailBuffer(lastMaterialCount);
        if (lastMaterialCount > 0) {
            uploadMaterialBuffer(g_MaterialGPU.data(), lastMaterialCount);
        }
    }

    // --- SELECTION PASS ---
    if (m_selectionFBO != 0 && selectionShader) {
        GLint currentFBO;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFBO);
        
        GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        GLboolean ditherEnabled = glIsEnabled(GL_DITHER);
        glDisable(GL_BLEND);
        glDisable(GL_DITHER);

        glBindFramebuffer(GL_FRAMEBUFFER, m_selectionFBO);
        glViewport(0, 0, m_viewportWidth, m_viewportHeight);
        
        // Bind selection shader FIRST, before any GL draw/clear ops on integer FBO
        selectionShader->use();
        selectionShader->useWithCamera(*camera_);
        
        int clearValue = -1;
        glClearBufferiv(GL_COLOR, 0, &clearValue);
        glClear(GL_DEPTH_BUFFER_BIT);
        
        std::vector<SceneNode*> selectableNodes;
        int currentID = 0;
        
        std::function<void(SceneNode*)> renderNodeSelection = [&](SceneNode* node) {
            selectableNodes.push_back(node);
            int nodeID = currentID++;
            selectionShader->setInt("u_EntityID", nodeID);
            selectionShader->setMat4("u_Model", node->globalTransform);
            
            for (auto& mesh : node->meshes) {
                glBindVertexArray(mesh.vao);
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, 0);
            }
            
            for (auto& child : node->children) {
                renderNodeSelection(child.get());
            }
        };
        
        for (auto& root : scene_->getRootNodes()) {
            renderNodeSelection(root.get());
        }
        
        // Draw lights for selection
        // We reuse LightGizmo shader since it already draws 6 vertices per light, 
        // wait, we can just use selectionShader with a small quad, but it's simpler 
        // to use lightGizmoShader with an ID output if we modify it...
        // Actually, just drawing quads with SelectionShader:
        selectionShader->setMat4("u_Model", glm::mat4(1.0f));
        // We can skip selecting lights from viewport for now, or just implement basic node picking.
        
        // Handle mouse click
        if (m_mouseClicked) {
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            int pixelData = -1;
            int mappedY = m_viewportHeight - m_mouseY - 1;
            if (m_mouseX >= 0 && m_mouseX < m_viewportWidth && mappedY >= 0 && mappedY < m_viewportHeight) {
                glReadPixels(m_mouseX, mappedY, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
            }
            
            if (pixelData != -1 && pixelData < selectableNodes.size()) {
                Editor::SetSelectedNode(selectableNodes[pixelData]);
            } else {
                Editor::SetSelectedNode(nullptr);
                Editor::SetSelectedLightIndex(-1);
            }
            m_mouseClicked = false;
        }
        
        if (blendEnabled) glEnable(GL_BLEND);
        if (ditherEnabled) glEnable(GL_DITHER);
        glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
    }
    // --- END SELECTION PASS ---

    // 0.5 Update Lights SSBO
    uploadLights();

    // Save current FBO to restore later
    GLint currentFBO;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFBO);

    // 0.5 Cascaded Shadow Map Pass
    std::vector<glm::mat4> lightSpaceMatrices;
    if (m_csmBuffer && shadowShader) {
        // Find first directional light
        glm::vec3 lightDir(0.0f, -1.0f, 0.1f); // Default pointing slightly down
        bool hasDirLight = false;
        for (const auto& light : scene_->getLights()) {
            if (light.position.w == 1.0f) { // Directional
                lightDir = glm::vec3(light.direction);
                hasDirLight = true;
                break;
            }
        }
        
        if (hasDirLight) {
            lightSpaceMatrices = getLightSpaceMatrices(lightDir);
            
            shadowShader->use();
            m_csmBuffer->Bind();
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT); // Peter panning fix
            
            for (size_t i = 0; i < lightSpaceMatrices.size(); ++i) {
                m_csmBuffer->BindForWriting(i);
                glClear(GL_DEPTH_BUFFER_BIT);
                shadowShader->setMat4("u_LightSpaceMatrix", lightSpaceMatrices[i]);
                for (auto sceneNode : scene_->getRootNodes()) {
                    renderNode(sceneNode.get(), shadowShader);
                }
            }
            
            glCullFace(GL_BACK); // Return to default
            m_csmBuffer->UnBind();
        }
    }

    // --- Update render context ---
    m_renderCtx.camera        = camera_;
    m_renderCtx.view          = camera_->GetViewMatrix();
    
    // TAA Jitter
    glm::mat4 projection = camera_->GetProjectionMatrix();
    // Always store the clean, unjittered projection — used for depth reconstruction and shadows.
    // Shadow coordinates must be computed from world-space positions that are reconstructed
    // using the unjittered inverse projection; using the jittered one shifts the reconstructed
    // position by a sub-pixel amount that changes every frame, breaking shadow comparisons.
    m_renderCtx.unjitteredProj = projection;
    if (m_renderCtx.taa.enabled) {
        m_renderCtx.taaFrameIndex = (m_renderCtx.taaFrameIndex + 1) % 2;
        m_renderCtx.taaJitterIndex = (m_renderCtx.taaJitterIndex + 1) % 16;
        
        // Halton sequence (bases 2 and 3)
        auto halton = [](int index, int base) {
            float f = 1, r = 0;
            while (index > 0) {
                f = f / base;
                r = r + f * (index % base);
                index = index / base;
            }
            return r;
        };
        
        float jitterX = (halton(m_renderCtx.taaJitterIndex + 1, 2) - 0.5f) * 2.0f / m_viewportWidth;
        float jitterY = (halton(m_renderCtx.taaJitterIndex + 1, 3) - 0.5f) * 2.0f / m_viewportHeight;
        
        projection[2][0] += jitterX; // Modify column 2, row 0
        projection[2][1] += jitterY; // Modify column 2, row 1
    }
    
    m_renderCtx.projection    = projection;
    m_renderCtx.cameraPos     = camera_->GetCameraPos();
    m_renderCtx.screenWidth   = m_viewportWidth;
    m_renderCtx.screenHeight  = m_viewportHeight;
    m_renderCtx.scene         = scene_;
    m_renderCtx.hasIBL        = m_envMap && m_envMap->IsLoaded();
    m_renderCtx.debugMode     = m_debugMode;
    m_renderCtx.visualizeTiles = visualizeTiles;
    m_renderCtx.cascadeCount  = m_shadowCascadeLevels.size() + 1;
    for (size_t i = 0; i < m_shadowCascadeLevels.size(); ++i) {
        m_renderCtx.cascadePlaneDistances[i] = m_shadowCascadeLevels[i];
    }
    m_renderCtx.cascadePlaneDistances[m_shadowCascadeLevels.size()] = 100.0f; // camera far plane
    for (size_t i = 0; i < lightSpaceMatrices.size(); ++i) {
        m_renderCtx.lightSpaceMatrices[i] = lightSpaceMatrices[i];
    }
    m_renderCtx.csmTextureArray = m_csmBuffer ? m_csmBuffer->GetTextureId() : 0;
    m_renderCtx.workGroupsX = m_workGroupsX;
    m_renderCtx.workGroupsY = m_workGroupsY;

    if (m_renderCtx.hasIBL) {
        m_renderCtx.irradianceMap = m_envMap->GetIrradianceMap();
        m_renderCtx.prefilterMap  = m_envMap->GetPrefilterMap();
        m_renderCtx.brdfLUT       = m_envMap->GetBrdfLUT();
    }

    // 1. Deferred Geometry Pass (renders to G-Buffer)
    if (m_geometryPass) m_geometryPass->Execute(m_renderCtx);

    // 2. Light Culling Compute Pass (reads G-Buffer Depth)
    if (m_cullingPass) m_cullingPass->Execute(m_renderCtx);

    // 3. SSAO Pass (reads G-Buffer Depth + Normal)
    if (m_ssaoPass) m_ssaoPass->Execute(m_renderCtx);

    // 4. Deferred Lighting Pass (reads G-Buffer, writes to HDR FBO)
    if (m_lightingPass) m_lightingPass->Execute(m_renderCtx);

    // Bind HDR FBO to draw forward elements
    glBindFramebuffer(GL_FRAMEBUFFER, m_renderCtx.hdrFBO);
    glViewport(0, 0, m_viewportWidth, m_viewportHeight);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // 5. Render Skybox (into HDR FBO)
    if (scene_->IsSkyboxDirty()) {
        loadSkybox(scene_->GetSkyboxPath());
        scene_->ClearSkyboxDirty();
    }
    if (m_skyboxPass) m_skyboxPass->Execute(m_renderCtx);
    glDepthFunc(GL_LESS);

    // 6. Outline Pass (Requires rendering geometry again to stencil, simplified for now)
    if (Editor::GetSelectedNode() && outlineShader) {
        glClear(GL_STENCIL_BUFFER_BIT);
        glEnable(GL_STENCIL_TEST);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);

        // We need a basic shader to render the selected node to stencil
        // For now, we can just use outlineShader and ignore its color output
        outlineShader->useWithCamera(*camera_);
        renderNode(Editor::GetSelectedNode(), outlineShader);
        
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        outlineShader->use();
        outlineShader->setMat4("u_Projection", m_renderCtx.projection);
        outlineShader->setMat4("u_View", m_renderCtx.view);
        outlineShader->setVec3("u_Color", glm::vec3(1.0, 0.5, 0.0)); // outline color
        
        glDisable(GL_DEPTH_TEST);
        renderNode(Editor::GetSelectedNode(), outlineShader);
        
        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
    }

    // 7. Render Light Gizmos (Icons)
    if (!scene_->getLights().empty()) {
        enableBlending(true);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glBindVertexArray(m_emptyVAO);
        lightGizmoShader->useWithCamera(*camera_);
    
        if (m_PointLightIcon) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_PointLightIcon);
            lightGizmoShader->setInt("u_PointLightIcon", 0);
        }
        if (m_DirLightIcon) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_DirLightIcon);
            lightGizmoShader->setInt("u_DirectionalLightIcon", 1);
        }

        glDrawArrays(GL_TRIANGLES, 0, scene_->getLights().size() * 6);
        glBindVertexArray(0);
        enableBlending(false);
    }

    // 8. Render Grid (into HDR FBO)
    if (grid) {
        grid->render(*camera_, deltaTime);
    }

    // 8.5. TAA Pass (reads HDR, writes resolved HDR)
    if (m_taaPass) m_taaPass->Execute(m_renderCtx);

    // 9. Bloom Pass (reads HDR color, writes bloom mip chain)
    if (m_bloomPass) m_bloomPass->Execute(m_renderCtx);

    // 10. Tone Mapping Pass — writes to the viewport FBO
    // Restore the viewport's original FBO (the one we saved)
    glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
    glViewport(0, 0, m_viewportWidth, m_viewportHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    if (m_toneMapPass) m_toneMapPass->Execute(m_renderCtx);

    // Store current matrices as previous for next frame
    m_renderCtx.prevView = m_renderCtx.view;
    m_renderCtx.prevProjection = m_renderCtx.projection;
}

void Renderer::renderNode(SceneNode* node, Pipeline* shader) {
    if (shader == testPipeline) {
        if (node == Editor::GetSelectedNode()) {
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
        } else {
            glStencilFunc(GL_ALWAYS, 0, 0xFF);
        }
    }

    shader->setMat4("u_Model", node->globalTransform);

    for (auto& mesh : node->meshes) {
        if (shader == testPipeline) {
            shader->setMaterial(mesh.materialIndex);
        }
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, 0);
    }
    for (auto child : node->children) {
        renderNode(child.get(), shader);
    }
}

void Renderer::setupForwardPlus(int width, int height) {
    m_viewportWidth  = width;
    m_viewportHeight = height;

    if (m_selectionTexture != 0) {
        glBindTexture(GL_TEXTURE_2D, m_selectionTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, width, height, 0, GL_RED_INTEGER, GL_INT, NULL);
        glBindTexture(GL_TEXTURE_2D, m_selectionDepth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Tile size is 16x16
    const int TILE_SIZE = 16;
    m_workGroupsX = (width + TILE_SIZE - 1) / TILE_SIZE;
    m_workGroupsY = (height + TILE_SIZE - 1) / TILE_SIZE;
    size_t numTiles = m_workGroupsX * m_workGroupsY;

    // Depth Prepass FBO
    if (m_depthMapFBO != 0) glDeleteFramebuffers(1, &m_depthMapFBO); m_depthMapFBO = 0;
    if (m_depthMap != 0) glDeleteTextures(1, &m_depthMap); m_depthMap = 0;

    glGenFramebuffers(1, &m_depthMapFBO);
    glGenTextures(1, &m_depthMap);
    
    glBindTexture(GL_TEXTURE_2D, m_depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLint prevFBO;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevFBO);

    glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);

    // Visible Light Indices SSBO
    const int maxLightsPerTile = 256;
    size_t visibleLightIndicesSize = numTiles * (1 + maxLightsPerTile) * sizeof(int);

    if (m_visibleLightIndicesSSBO != 0) glDeleteBuffers(1, &m_visibleLightIndicesSSBO);
    glGenBuffers(1, &m_visibleLightIndicesSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_visibleLightIndicesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, visibleLightIndicesSize, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_visibleLightIndicesSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Resize HDR pipeline
    CreateHDRFramebuffer(m_renderCtx, width, height);
}

void Renderer::uploadLights() {
    if (!scene_) return;
    
    const auto& lights = scene_->getLights();
    size_t lightCount = lights.size();
    if (lightCount == 0) lightCount = 1; // Prevent empty buffer error

    if (m_lightsSSBO == 0) {
        glGenBuffers(1, &m_lightsSSBO);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_lightsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, lightCount * sizeof(Light), lights.empty() ? nullptr : lights.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_lightsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

std::vector<glm::vec4> Renderer::getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view) {
    const auto inv = glm::inverse(proj * view);
    std::vector<glm::vec4> frustumCorners;
    for (unsigned int x = 0; x < 2; ++x) {
        for (unsigned int y = 0; y < 2; ++y) {
            for (unsigned int z = 0; z < 2; ++z) {
                const glm::vec4 pt = 
                    inv * glm::vec4(
                        2.0f * x - 1.0f,
                        2.0f * y - 1.0f,
                        2.0f * z - 1.0f,
                        1.0f);
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }
    return frustumCorners;
}

glm::mat4 Renderer::getLightSpaceMatrix(const float nearPlane, const float farPlane, const glm::vec3& lightDir) {
    // 1. Build the true frustum for this cascade using the camera's FOV and aspect ratio
    const auto proj = glm::perspective(
        glm::radians(camera_->GetFov()), 
        camera_->GetAspect(), 
        nearPlane, 
        farPlane
    );
    const auto corners = getFrustumCornersWorldSpace(proj, camera_->GetViewMatrix());

    // 2. Find the center of the cascade frustum bounding sphere
    glm::vec3 center = glm::vec3(0, 0, 0);
    for (const auto& v : corners) {
        center += glm::vec3(v);
    }
    center /= corners.size();

    // 3. Find the radius of the bounding sphere
    float radius = 0.0f;
    for (const auto& v : corners) {
        float distance = glm::length(glm::vec3(v) - center);
        radius = glm::max(radius, distance);
    }
    // Round radius to prevent sub-pixel shimmering (optional but good practice)
    radius = std::ceil(radius * 16.0f) / 16.0f;

    // Prevent degenerate lookAt matrix when light direction is near-parallel to up vector
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (glm::abs(glm::dot(glm::normalize(lightDir), up)) > 0.99f) {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    // 4. Pull the light back along the light direction by the radius
    glm::vec3 lightPos = center - glm::normalize(lightDir) * radius;
    const auto lightView = glm::lookAt(
        lightPos, 
        center, 
        up
    );

    // 5. Build an orthographic projection that perfectly bounds the sphere
    float minX = -radius;
    float maxX = radius;
    float minY = -radius;
    float maxY = radius;
    
    // Expand the Z bounds significantly. 
    // Pulling the near plane back catches shadow casters behind the camera.
    // Pushing the far plane forward ensures we don't clip the back half of the cascade sphere.
    constexpr float zMultiplier = 10.0f;
    float minZ = -radius * zMultiplier;
    float maxZ = radius * zMultiplier;

    const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
    return lightProjection * lightView;
}

std::vector<glm::mat4> Renderer::getLightSpaceMatrices(const glm::vec3& lightDir) {
    std::vector<glm::mat4> ret;
    for (size_t i = 0; i < m_shadowCascadeLevels.size() + 1; ++i) {
        if (i == 0) {
            ret.push_back(getLightSpaceMatrix(0.1f, m_shadowCascadeLevels[i], lightDir));
        } else if (i < m_shadowCascadeLevels.size()) {
            ret.push_back(getLightSpaceMatrix(m_shadowCascadeLevels[i - 1], m_shadowCascadeLevels[i], lightDir));
        } else {
            ret.push_back(getLightSpaceMatrix(m_shadowCascadeLevels[i - 1], 100.0f, lightDir)); // 100.0 is camera far plane
        }
    }
    return ret;
}

} // namespace lgt
