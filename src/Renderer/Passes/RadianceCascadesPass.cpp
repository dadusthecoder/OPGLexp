#include "RadianceCascadesPass.h"
#include "../../Vendor/glad.h"
#include "../Core/RayTracingSubsystem.h"
#include "../../Helpers/Logger.h"
#include "../../Helpers/GPUTimer.h"
#include "../Validation/RendererValidationFramework.h"
#include <vector>

namespace lgt {

    Shader* RadianceCascadesPass::s_TraceShader = nullptr;
    Shader* RadianceCascadesPass::s_MergeShader = nullptr;
    Shader* RadianceCascadesPass::s_TemporalBlendShader = nullptr;
    Shader* RadianceCascadesPass::s_DebugShader = nullptr;

    Texture* RadianceCascadesPass::s_RadianceAtlas = nullptr;
    Texture* RadianceCascadesPass::s_HistoryAtlas = nullptr;

    uint32_t RadianceCascadesPass::s_Width = 0;
    uint32_t RadianceCascadesPass::s_Height = 0;

    int RadianceCascadesPass::s_CascadeCount = 6;
    int RadianceCascadesPass::s_BaseProbeSpacing = 4;
    float RadianceCascadesPass::s_BaseInterval = 0.05f;
    float RadianceCascadesPass::s_RayIntensity = 1.0f;
    int RadianceCascadesPass::s_DebugCascade = -1;

#ifdef ATLAS_VALIDATION
    static int s_DebugCategory = 0;
    static int s_DebugMode = 0;
    static std::vector<RadianceCascadesPass::CascadeStats> s_CascadeStats;

    int& RadianceCascadesPass::GetDebugCategory() { return s_DebugCategory; }
    int& RadianceCascadesPass::GetDebugMode() { return s_DebugMode; }
    RadianceCascadesPass::CascadeStats* RadianceCascadesPass::GetStatistics() { 
        if (s_CascadeStats.size() != s_CascadeCount) s_CascadeStats.resize(s_CascadeCount);
        return s_CascadeStats.data(); 
    }
#endif

    void RadianceCascadesPass::Init(uint32_t width, uint32_t height) {
        s_Width = width;
        s_Height = height;

        s_TraceShader = Shader::CreateCompute("res/shaders/rc_trace.comp");
        s_MergeShader = Shader::CreateCompute("res/shaders/rc_merge.comp");
        s_TemporalBlendShader = Shader::CreateCompute("res/shaders/rc_temporal_blend.comp");
        s_DebugShader = Shader::CreateCompute("res/shaders/rc_debug.comp");

        RecreateAtlas();
        CORE_INFO("RadianceCascadesPass initialized.");

#ifdef ATLAS_VALIDATION
        GLint maxComputeWorkGroupSize[3];
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxComputeWorkGroupSize[0]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxComputeWorkGroupSize[1]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxComputeWorkGroupSize[2]);
        GLint maxComputeWorkGroupInvocations;
        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxComputeWorkGroupInvocations);
        GLint maxTextureSize;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
        
        CORE_INFO("========== RC Resources ==========");
        CORE_INFO("GPU Limits:");
        CORE_INFO("Max Compute WorkGroup Size: {}x{}x{}", maxComputeWorkGroupSize[0], maxComputeWorkGroupSize[1], maxComputeWorkGroupSize[2]);
        CORE_INFO("Max Compute Invocations: {}", maxComputeWorkGroupInvocations);
        CORE_INFO("Max Texture Size: {}", maxTextureSize);
        CORE_INFO("Atlas: Format=RGBA16F, Size={}x{}", s_Width, s_Height * s_CascadeCount);
        CORE_INFO("==================================");
#endif
    }

    void RadianceCascadesPass::RecreateAtlas() {
        if (s_RadianceAtlas) {
            delete s_RadianceAtlas;
            delete s_HistoryAtlas;
        }

        TextureDescriptor desc;
        desc.width = s_Width;
        desc.height = s_Height * s_CascadeCount; // Atlas with N cascades laid out vertically
        desc.format = TextureFormat::RGBA16F;
        desc.wrapS = TextureWrap::ClampToEdge;
        desc.wrapT = TextureWrap::ClampToEdge;
        desc.minFilter = TextureFilter::Nearest; // We want manual fetching
        desc.magFilter = TextureFilter::Nearest;
        desc.generateMipmaps = false;

        s_RadianceAtlas = Texture::Create(desc);
        s_HistoryAtlas = Texture::Create(desc);
        
        CORE_INFO("Radiance Cascades Atlas recreated: {}x{} ({} cascades)", desc.width, desc.height, s_CascadeCount);
    }

    void RadianceCascadesPass::Execute(uint32_t gDepthID, uint32_t gNormalID, const glm::mat4& invVP, const glm::vec3& cameraPos, int frameIndex, const glm::mat4& prevInvVP, const glm::vec3& sunDir, const glm::vec3& sunColor, float sunIntensity) {
        if (!RayTracingSubsystem::IsReady()) return;

        s_TraceShader->Bind();
        
        s_TraceShader->SetInt("u_CascadeCount", s_CascadeCount);
        s_TraceShader->SetInt("u_BaseProbeSpacing", s_BaseProbeSpacing);
        s_TraceShader->SetFloat("u_BaseInterval", s_BaseInterval);
        s_TraceShader->SetInt("u_ScreenWidth", s_Width);
        s_TraceShader->SetInt("u_ScreenHeight", s_Height);
        s_TraceShader->SetFloat("u_RayIntensity", s_RayIntensity);
        
        s_TraceShader->SetMat4("u_InvViewProjection", invVP);
        s_TraceShader->SetFloat3("u_CameraPos", cameraPos);
        
        s_TraceShader->SetFloat3("u_SunDirection", sunDir);
        s_TraceShader->SetFloat3("u_SunColor", sunColor);
        s_TraceShader->SetFloat("u_SunIntensity", sunIntensity);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gDepthID);
        s_TraceShader->SetInt("u_gDepth", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormalID);
        s_TraceShader->SetInt("u_gNormal", 1);

        s_RadianceAtlas->BindImage(0, 0, false, 0, TextureAccess::WriteOnly);
        RayTracingSubsystem::Bind(6, 7);
        
#ifdef ATLAS_VALIDATION
        s_TraceShader->SetInt("u_DebugCategory", s_DebugCategory);
        s_TraceShader->SetInt("u_DebugMode", s_DebugMode);
#endif

        // Dispatch trace for all cascades
        uint32_t numGroupsX = (s_Width + 7) / 8;
        uint32_t numGroupsY = ((s_Height * s_CascadeCount) + 7) / 8;
        
        {
            ScopedGPUTimer timer("RC Trace");
            glDispatchCompute(numGroupsX, numGroupsY, 1);
        }
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // --- 2. Temporal Blend (Optional/TODO implementation in shader) ---
        // Will blend s_RadianceAtlas with s_HistoryAtlas

        // --- 3. Merge Cascades ---
        s_MergeShader->Bind();
        s_MergeShader->SetInt("u_CascadeCount", s_CascadeCount);
        s_MergeShader->SetInt("u_BaseProbeSpacing", s_BaseProbeSpacing);
        s_MergeShader->SetInt("u_ScreenWidth", s_Width);
        s_MergeShader->SetInt("u_ScreenHeight", s_Height);
        
#ifdef ATLAS_VALIDATION
        s_MergeShader->SetInt("u_DebugCategory", s_DebugCategory);
        s_MergeShader->SetInt("u_DebugMode", s_DebugMode);
#endif
        
        // We do in-place merging or ping-pong. 
        // Since we merge from N-1 down to 0, cascade i reads from cascade i+1.
        // If we bind the whole atlas as read-write, we can just do one dispatch per cascade,
        // starting from N-2 down to 0.
        s_RadianceAtlas->BindImage(0, 0, false, 0, TextureAccess::ReadWrite);

        for (int i = s_CascadeCount - 2; i >= 0; i--) {
            s_MergeShader->SetInt("u_CurrentCascade", i);
            uint32_t gy = (s_Height + 7) / 8;
            {
                ScopedGPUTimer timer("RC Merge");
                glDispatchCompute(numGroupsX, gy, 1);
            }
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

        // --- 4. History Update ---
        // Copy resolved Cascade 0 (or all cascades) to HistoryAtlas for next frame
        // (can be done with glCopyImageSubData for speed)
        glCopyImageSubData(s_RadianceAtlas->GetRendererID(), GL_TEXTURE_2D, 0, 0, 0, 0,
                           s_HistoryAtlas->GetRendererID(), GL_TEXTURE_2D, 0, 0, 0, 0,
                           s_Width, s_Height * s_CascadeCount, 1);
    }

    void RadianceCascadesPass::Resize(uint32_t width, uint32_t height) {
        s_Width = width;
        s_Height = height;
        RecreateAtlas();
    }

    void RadianceCascadesPass::Shutdown() {
        delete s_TraceShader; s_TraceShader = nullptr;
        delete s_MergeShader; s_MergeShader = nullptr;
        delete s_TemporalBlendShader; s_TemporalBlendShader = nullptr;
        delete s_DebugShader; s_DebugShader = nullptr;
        delete s_RadianceAtlas; s_RadianceAtlas = nullptr;
        delete s_HistoryAtlas; s_HistoryAtlas = nullptr;
    }

    uint32_t RadianceCascadesPass::GetRadianceAtlasID() {
        return s_RadianceAtlas ? s_RadianceAtlas->GetRendererID() : 0;
    }

    int& RadianceCascadesPass::GetCascadeCount() { return s_CascadeCount; }
    int& RadianceCascadesPass::GetBaseProbeSpacing() { return s_BaseProbeSpacing; }
    float& RadianceCascadesPass::GetBaseInterval() { return s_BaseInterval; }
    float& RadianceCascadesPass::GetRayIntensity() { return s_RayIntensity; }
    int& RadianceCascadesPass::GetDebugCascade() { return s_DebugCascade; }
}
