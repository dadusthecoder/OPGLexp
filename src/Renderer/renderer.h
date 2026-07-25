#pragma once
#include <cstdint>
#include <iostream>
#include <memory>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "Vendor/glad.h"
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // For transformations like translate, rotate, perspective
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "Texture.h"
#include "Shader.h"
#include "Material.h"
#include "RenderPass.h"
#include "ToneMapPass.h"
#include "BloomPass.h"
#include "SSAOPass.h"
#include "SkyboxPass.h"
#define SHADOW_WIDTH  2048
#define SHADOW_HEIGHT 2048

namespace lgt {
class Camera;
class Renderer;
class Scene;
class EnvironmentMap;
class DeferredGeometryPass;
class LightCullingPass;
class DeferredLightingPass;
class TAAPass;
class ToneMapPass;
class BloomPass;
class SSAOPass;
class SkyboxPass;
struct SceneNode;
using RenderId = unsigned int;



struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec4 tangent;
};

struct Mesh {
    std::string name;
    GLuint   vao, vbo, ibo;
    size_t   indexCount;
    uint32_t materialIndex;

    void setup(const std::string& meshName, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, uint32_t matIdx) {
        name          = meshName;
        materialIndex = matIdx;
        indexCount    = indices.size();

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ibo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        // Normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        // TexCoords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
        // Tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

        glBindVertexArray(0);
    }
};

struct GridSettings {
    glm::vec3 baseColor       = glm::vec3(0.1f, 0.1f, 0.15f);
    glm::vec3 gradientColor   = glm::vec3(0.2f, 0.4f, 0.8f);
    float     fadeDistance    = 50.0f;
    float     gridIntensity   = 0.5f;
    float     waveAmplitude   = 0.5f;
    float     waveFrequency   = 0.1f;
    bool      enableAnimation = false;
    bool      enableGrid      = true;
    bool      enableGradient  = true;
};

class FrameBuffer {
private:
    int      m_width, m_height;
    RenderId m_FBO;
    RenderId m_textureId;
    RenderId m_renderbuffer;
    friend class Renderer;

public:
    void Use();
    void Unuse();

    RenderId GetTextureId();
    RenderId GetHeight();
    RenderId GetWidth();
    void     Resize(int w, int h);

    FrameBuffer(int w, int h);
    ~FrameBuffer();
};

class DepthBuffer {
private:
    RenderId m_BufferId;
    RenderId m_textureId;

public:
    DepthBuffer();
    ~DepthBuffer();
    RenderId GetTextureId();

    void BindTex(unsigned int unit);
    void UnBindTex();
    void Use();
    void Unsue();
    void Bind();
    void UnBind();
};

class Grid {

private:
    std::unique_ptr<Pipeline> m_gridShader;
    GLuint                    m_VAO, m_VBO, m_EBO;
    std::vector<float>        m_vertices;
    std::vector<unsigned int> m_indices;

    // Animation settings
    GridSettings m_settings;

    float m_time = 0.0f;

public:
    Grid();
    ~Grid();
    void          generateGridMesh(int width, int height, float spacing);
    void          setupBuffers();
    void          render(Camera& cam, float deltaTime);
    GridSettings& getSetting();
    void          cleanup();
};

enum class RenderMode {
    FILL,
    WIREFRAME,
    POINT
};

// OpenGL debug macro
#define GlCall(x)                                                                                                                \
    lgt::Renderer::GLClearError();                                                                                               \
    x;                                                                                                                           \
    if (!lgt::Renderer::GLLogCall(#x, __FILE__, __LINE__))                                                                       \
        __debugbreak();

class CascadedShadowBuffer {
private:
    RenderId m_BufferId;
    RenderId m_textureId;
    int m_width, m_height;

public:
    CascadedShadowBuffer(int width, int height, int cascadeCount);
    ~CascadedShadowBuffer();
    RenderId GetTextureId();

    void Bind();
    void UnBind();
    void BindTex(unsigned int unit);
    void UnBindTex();
    void BindForWriting(int layer);
};

class Renderer {
public:
    Renderer(Scene* scene, Camera* camera);

    ~Renderer();

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    Renderer(Renderer&&)            = default;
    Renderer& operator=(Renderer&&) = default;

    void init();
    void shutdown();

    static void GLClearError();
    static bool GLLogCall(const char* function, const char* file, int line);

    void Clear(const glm::vec3& backgroundColor = glm::vec3(0.0f)) const;

    void Initauad();
    void renderQuad();

    float updateAndLogFPS(GLFWwindow* window);
    float getFPS() const { return m_fps; }

    void enableDepthTesting(bool enable = true);
    void enableBlending(bool enable = true);

    RenderMode getRenderMode() const { return m_currentRenderMode; }

    void setRenderMode(RenderMode mode);
    void setViewport(int width, int height);
    void setCamera(Camera* camera);
    void setScene(Scene* Scene);
    void setDebugMode(DebugMode mode);
    DebugMode getDebugMode() const { return m_debugMode; }
    
    bool visualizeTiles = false;

    // material management
    void createMaterailBuffer(size_t size);
    void uploadMaterialBuffer(MaterialGPU* data, size_t count);
    void updateMaterial(const MaterialGPU& mat, uint32_t index);

    void updateDirtyRange(MaterialGPU* data, size_t offset, size_t count);

    void render(class Grid* grid = nullptr, float deltaTime = 0.0f);
    void renderNode(SceneNode* node, Pipeline* shader);

    void setMouseSelection(int x, int y) { m_mouseX = x; m_mouseY = y; m_mouseClicked = true; }

    // Post-processing settings (exposed for ImGui)
    RenderContext& getRenderContext() { return m_renderCtx; }

private:
    // Render Graph (post-processing pipeline)
    RenderContext m_renderCtx;
    RenderGraph   m_renderGraph;
    
    DeferredGeometryPass* m_geometryPass = nullptr;
    LightCullingPass*     m_cullingPass  = nullptr;
    SSAOPass*             m_ssaoPass     = nullptr;
    DeferredLightingPass* m_lightingPass = nullptr;
    SkyboxPass*           m_skyboxPass   = nullptr;
    BloomPass*            m_bloomPass    = nullptr;
    ToneMapPass*          m_toneMapPass  = nullptr;
    TAAPass*              m_taaPass      = nullptr;
    // Helper methods
    static const char* getGLErrorString(GLenum error);

    // Performance monitoring
    mutable int    m_frameCount;
    mutable float  m_fps;
    mutable double m_lastTime;

    RenderId quadVAO = 0;
    RenderId quadVBO;

    // Render state
    RenderMode m_currentRenderMode;
    DebugMode m_debugMode = DebugMode::FINAL_COLOR;

    Pipeline* testPipeline = nullptr;
    Pipeline* depthPrepassShader = nullptr;
    Pipeline* lightCullingShader = nullptr;
    Pipeline* lightGizmoShader = nullptr;
    Pipeline* selectionShader = nullptr;
    Pipeline* outlineShader = nullptr;

    Camera*   camera_      = nullptr;
    Scene*    scene_       = nullptr;

    // Forward+ resources
    GLuint m_lightsSSBO              = 0;
    GLuint m_visibleLightIndicesSSBO = 0;
    GLuint m_PointLightIcon          = 0;
    GLuint m_DirLightIcon            = 0;
    RenderId m_depthMapFBO = 0;
    RenderId m_depthMap = 0;
    
    int m_viewportWidth = 0;
    int m_viewportHeight = 0;
    int m_workGroupsX = 0;
    int m_workGroupsY = 0;
    GLuint m_emptyVAO = 0; // Reusable empty VAO for attribute-less draws (gizmos)

    void setupForwardPlus(int width, int height);
    void uploadLights();

    // Skybox & IBL
    std::unique_ptr<EnvironmentMap> m_envMap;
    
    EnvironmentMap* getEnvironmentMap() { return m_envMap.get(); }
    void loadSkybox(const std::string& hdrPath);

    // Cascaded Shadow Maps
    GLuint m_selectionFBO = 0;
    GLuint m_selectionTexture = 0;
    GLuint m_selectionDepth = 0;
    
    int m_mouseX = -1;
    int m_mouseY = -1;
    bool m_mouseClicked = false;
    
    std::unique_ptr<CascadedShadowBuffer> m_csmBuffer;
    Pipeline* shadowShader = nullptr;
    const std::vector<float>& getCascadeLevels() const { return m_shadowCascadeLevels; }
    std::vector<float> m_shadowCascadeLevels;
    
    std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view);
    glm::mat4 getLightSpaceMatrix(const float nearPlane, const float farPlane, const glm::vec3& lightDir);
    std::vector<glm::mat4> getLightSpaceMatrices(const glm::vec3& lightDir);
};
} // namespace lgt
