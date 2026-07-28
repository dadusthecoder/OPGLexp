#include "DDGIPass.h"
#include "../../Vendor/glad.h"
#include "BVHPass.h"

namespace lgt {

    Shader*  DDGIPass::s_TraceShader = nullptr;
    Shader*  DDGIPass::s_UpdateShader = nullptr;
    Buffer*  DDGIPass::s_RayDataBuffer = nullptr;
    Texture* DDGIPass::s_IrradianceAtlas = nullptr;
    
    glm::ivec3 DDGIPass::s_GridSize = glm::ivec3(0);
    glm::vec3  DDGIPass::s_ProbeOrigin = glm::vec3(0.0f);
    glm::vec3  DDGIPass::s_ProbeSpacing = glm::vec3(0.0f);
    int DDGIPass::s_TotalProbes = 0;
    int DDGIPass::s_RaysPerProbe = 64;

    void DDGIPass::Init(glm::ivec3 gridSize, glm::vec3 probeOrigin, glm::vec3 probeSpacing) {
        s_GridSize = gridSize;
        s_ProbeOrigin = probeOrigin;
        s_ProbeSpacing = probeSpacing;
        s_TotalProbes = gridSize.x * gridSize.y * gridSize.z;

        s_TraceShader = Shader::CreateCompute("res/shaders/ddgi_probe_trace.comp");
        s_UpdateShader = Shader::CreateCompute("res/shaders/ddgi_probe_update.comp");

        s_RayDataBuffer = Buffer::Create(BufferType::ShaderStorageBuffer, s_TotalProbes * s_RaysPerProbe * sizeof(glm::vec4) * 2, nullptr, BufferUsage::DynamicDraw);

        TextureDescriptor desc;
        desc.width = s_GridSize.x * s_GridSize.y * 10;
        desc.height = s_GridSize.z * 10;
        desc.format = TextureFormat::RGBA16F;
        desc.wrapS = TextureWrap::ClampToEdge;
        desc.wrapT = TextureWrap::ClampToEdge;
        desc.minFilter = TextureFilter::Linear;
        desc.magFilter = TextureFilter::Linear;
        desc.generateMipmaps = false;
        
        s_IrradianceAtlas = Texture::Create(desc);
    }

    void DDGIPass::Execute(glm::vec3 sunDir, glm::vec3 sunColor, float sunIntensity, int frameIndex) {
        if (!BVHPass::IsReady()) return;

        s_TraceShader->Bind();
        s_TraceShader->SetInt("u_RaysPerProbe", s_RaysPerProbe);
        s_TraceShader->SetInt("u_TotalProbes", s_TotalProbes);
        s_TraceShader->SetInt("u_FrameIndex", frameIndex);
        s_TraceShader->SetFloat3("u_SunDir", sunDir);
        s_TraceShader->SetFloat3("u_SunColor", sunColor);
        s_TraceShader->SetFloat("u_SunIntensity", sunIntensity);
        int traceGridSize[3] = {s_GridSize.x, s_GridSize.y, s_GridSize.z};
        s_TraceShader->SetIntArray("u_GridSize", traceGridSize, 3);
        s_TraceShader->SetFloat3("u_ProbeOrigin", s_ProbeOrigin);
        s_TraceShader->SetFloat3("u_ProbeSpacing", s_ProbeSpacing);

        s_RayDataBuffer->BindBase(8);
        BVHPass::Bind(6, 7);
        
        glDispatchCompute((s_TotalProbes + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        s_UpdateShader->Bind();
        s_UpdateShader->SetInt("u_RaysPerProbe", s_RaysPerProbe);
        int gridSize[3] = {s_GridSize.x, s_GridSize.y, s_GridSize.z};
        s_UpdateShader->SetIntArray("u_GridSize", gridSize, 3);
        
        s_RayDataBuffer->BindBase(8);
        s_IrradianceAtlas->BindImage(0, 0, false, 0, TextureAccess::ReadWrite);
        
        glDispatchCompute(s_GridSize.x * s_GridSize.y, s_GridSize.z, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    void DDGIPass::Resize(uint32_t width, uint32_t height) {
        // No-op
    }

    void DDGIPass::Shutdown() {
        delete s_RayDataBuffer; s_RayDataBuffer = nullptr;
        delete s_IrradianceAtlas; s_IrradianceAtlas = nullptr;
        delete s_TraceShader; s_TraceShader = nullptr;
        delete s_UpdateShader; s_UpdateShader = nullptr;
    }

    uint32_t DDGIPass::GetIrradianceAtlasID() {
        return s_IrradianceAtlas ? s_IrradianceAtlas->GetRendererID() : 0;
    }

    glm::ivec3 DDGIPass::GetGridSize() { return s_GridSize; }
    glm::vec3  DDGIPass::GetProbeOrigin() { return s_ProbeOrigin; }
    glm::vec3  DDGIPass::GetProbeSpacing() { return s_ProbeSpacing; }
}
