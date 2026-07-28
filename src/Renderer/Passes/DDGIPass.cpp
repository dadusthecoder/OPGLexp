#include "DDGIPass.h"
#include "../../Vendor/glad.h"
#include "BVHPass.h"
#include "../../Helpers/Logger.h"

namespace lgt {

    Shader*  DDGIPass::s_TraceShader      = nullptr;
    Shader*  DDGIPass::s_UpdateShader     = nullptr;
    Shader*  DDGIPass::s_BorderCopyShader = nullptr;
    Shader*  DDGIPass::s_ClassifyShader   = nullptr;
    Buffer*  DDGIPass::s_RayDataBuffer    = nullptr;
    Buffer*  DDGIPass::s_ProbeStateBuffer = nullptr;
    Texture* DDGIPass::s_IrradianceAtlas  = nullptr;
    Texture* DDGIPass::s_DistanceAtlas    = nullptr;

    glm::ivec3 DDGIPass::s_GridSize     = glm::ivec3(0);
    glm::vec3  DDGIPass::s_ProbeOrigin  = glm::vec3(0.0f);
    glm::vec3  DDGIPass::s_ProbeSpacing = glm::vec3(0.0f);
    int DDGIPass::s_TotalProbes  = 0;
    int DDGIPass::s_RaysPerProbe = 64;
    int DDGIPass::s_ProbesPerRow = 0;

    void DDGIPass::Init(glm::ivec3 gridSize, glm::vec3 probeOrigin, glm::vec3 probeSpacing) {
        s_GridSize     = gridSize;
        s_ProbeOrigin  = probeOrigin;
        s_ProbeSpacing = probeSpacing;
        s_TotalProbes  = gridSize.x * gridSize.y * gridSize.z;
        s_ProbesPerRow = gridSize.x * gridSize.z;
        int numRows    = gridSize.y;

        CORE_INFO("DDGIPass: Init grid=({},{},{}), total={}, probesPerRow={}, numRows={}",
                  gridSize.x, gridSize.y, gridSize.z, s_TotalProbes, s_ProbesPerRow, numRows);

        // Create compute shaders
        s_TraceShader      = Shader::CreateCompute("res/shaders/ddgi_probe_trace.comp");
        s_UpdateShader     = Shader::CreateCompute("res/shaders/ddgi_probe_update.comp");
        s_BorderCopyShader = Shader::CreateCompute("res/shaders/ddgi_border_copy.comp");
        s_ClassifyShader   = Shader::CreateCompute("res/shaders/ddgi_probe_classify.comp");

        // Ray data buffer: 2 vec4s per ray (radiance + direction)
        s_RayDataBuffer = Buffer::Create(
            BufferType::ShaderStorageBuffer,
            s_TotalProbes * s_RaysPerProbe * sizeof(glm::vec4) * 2,
            nullptr, BufferUsage::DynamicDraw);

        // Probe state buffer: 1 vec4 per probe (xyz=offset, w=state)
        // Initialize all probes as active with zero offset
        std::vector<glm::vec4> initialState(s_TotalProbes, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        s_ProbeStateBuffer = Buffer::Create(
            BufferType::ShaderStorageBuffer,
            s_TotalProbes * sizeof(glm::vec4),
            initialState.data(), BufferUsage::DynamicDraw);

        // Irradiance atlas: 8x8 tile per probe (6x6 inner + 1px border)
        {
            TextureDescriptor desc;
            desc.width  = s_ProbesPerRow * 8;
            desc.height = numRows * 8;
            desc.format = TextureFormat::RGBA16F;
            desc.wrapS  = TextureWrap::ClampToEdge;
            desc.wrapT  = TextureWrap::ClampToEdge;
            desc.minFilter = TextureFilter::Linear;
            desc.magFilter = TextureFilter::Linear;
            desc.generateMipmaps = false;
            s_IrradianceAtlas = Texture::Create(desc);
            CORE_INFO("DDGIPass: Irradiance atlas {}x{}", desc.width, desc.height);
        }

        // Distance atlas: 16x16 tile per probe (14x14 inner + 1px border)
        {
            TextureDescriptor desc;
            desc.width  = s_ProbesPerRow * 16;
            desc.height = numRows * 16;
            desc.format = TextureFormat::RG16F;
            desc.wrapS  = TextureWrap::ClampToEdge;
            desc.wrapT  = TextureWrap::ClampToEdge;
            desc.minFilter = TextureFilter::Linear;
            desc.magFilter = TextureFilter::Linear;
            desc.generateMipmaps = false;
            s_DistanceAtlas = Texture::Create(desc);
            CORE_INFO("DDGIPass: Distance atlas {}x{}", desc.width, desc.height);
        }
    }

    void DDGIPass::Execute(glm::vec3 sunDir, glm::vec3 sunColor, float sunIntensity, int frameIndex) {
        if (!BVHPass::IsReady()) return;

        // --- Pass 1: Probe Ray Tracing ---
        s_TraceShader->Bind();
        s_TraceShader->SetInt("u_RaysPerProbe", s_RaysPerProbe);
        s_TraceShader->SetInt("u_TotalProbes", s_TotalProbes);
        s_TraceShader->SetInt("u_FrameIndex", frameIndex);
        s_TraceShader->SetFloat3("u_SunDirection", sunDir);
        s_TraceShader->SetFloat3("u_SunColor", sunColor);
        s_TraceShader->SetFloat("u_SunIntensity", sunIntensity);
        s_TraceShader->SetFloat("u_MaxRayDistance", 50.0f);
        s_TraceShader->SetInt3("u_ProbeGridSize", s_GridSize);
        s_TraceShader->SetFloat3("u_ProbeOrigin", s_ProbeOrigin);
        s_TraceShader->SetFloat3("u_ProbeSpacing", s_ProbeSpacing);
        s_TraceShader->SetInt("u_ProbesPerRow", s_ProbesPerRow);

        s_RayDataBuffer->BindBase(8);
        s_ProbeStateBuffer->BindBase(10);
        BVHPass::Bind(6, 7);

        // Bind previous frame irradiance for multi-bounce (texture unit 8)
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, s_IrradianceAtlas->GetRendererID());
        s_TraceShader->SetInt("u_PrevIrradiance", 8);

        glDispatchCompute((s_TotalProbes + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // --- Pass 2: Irradiance Update ---
        s_UpdateShader->Bind();
        s_UpdateShader->SetInt("u_RaysPerProbe", s_RaysPerProbe);
        s_UpdateShader->SetFloat("u_Hysteresis", 0.97f);
        s_UpdateShader->SetFloat("u_GammaExponent", 5.0f);
        s_UpdateShader->SetInt3("u_ProbeGridSize", s_GridSize);
        s_UpdateShader->SetInt("u_TotalProbes", s_TotalProbes);
        s_UpdateShader->SetInt("u_ProbesPerRow", s_ProbesPerRow);
        s_UpdateShader->SetInt("u_Mode", 0); // Irradiance mode

        s_RayDataBuffer->BindBase(8);
        s_ProbeStateBuffer->BindBase(10);
        s_IrradianceAtlas->BindImage(0, 0, false, 0, TextureAccess::ReadWrite);

        glDispatchCompute(s_TotalProbes, 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // --- Pass 3: Distance Update ---
        s_UpdateShader->SetInt("u_Mode", 1); // Distance mode
        s_DistanceAtlas->BindImage(1, 0, false, 0, TextureAccess::ReadWrite);

        glDispatchCompute(s_TotalProbes, 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // --- Pass 4: Border Copy (Irradiance) ---
        s_BorderCopyShader->Bind();
        s_BorderCopyShader->SetInt("u_TileSize", 8);
        s_BorderCopyShader->SetInt("u_TotalProbes", s_TotalProbes);
        s_BorderCopyShader->SetInt("u_ProbesPerRow", s_ProbesPerRow);
        s_BorderCopyShader->SetInt3("u_ProbeGridSize", s_GridSize);
        s_BorderCopyShader->SetInt("u_Mode", 0);
        s_IrradianceAtlas->BindImage(0, 0, false, 0, TextureAccess::ReadWrite);

        glDispatchCompute(s_TotalProbes, 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // --- Pass 5: Border Copy (Distance) ---
        s_BorderCopyShader->SetInt("u_TileSize", 16);
        s_BorderCopyShader->SetInt("u_Mode", 1);
        s_DistanceAtlas->BindImage(1, 0, false, 0, TextureAccess::ReadWrite);

        glDispatchCompute(s_TotalProbes, 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // --- Pass 6: Probe Classification (every 4th frame to amortize) ---
        if (frameIndex % 4 == 0) {
            s_ClassifyShader->Bind();
            s_ClassifyShader->SetInt3("u_ProbeGridSize", s_GridSize);
            s_ClassifyShader->SetFloat3("u_ProbeSpacing", s_ProbeSpacing);
            s_ClassifyShader->SetInt("u_RaysPerProbe", s_RaysPerProbe);
            s_ClassifyShader->SetInt("u_TotalProbes", s_TotalProbes);

            s_RayDataBuffer->BindBase(8);
            s_ProbeStateBuffer->BindBase(10);

            glDispatchCompute((s_TotalProbes + 63) / 64, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
    }

    void DDGIPass::Resize(uint32_t width, uint32_t height) {
        // DDGI atlas dimensions are independent of viewport size
    }

    void DDGIPass::Shutdown() {
        delete s_RayDataBuffer;    s_RayDataBuffer    = nullptr;
        delete s_ProbeStateBuffer; s_ProbeStateBuffer = nullptr;
        delete s_IrradianceAtlas;  s_IrradianceAtlas  = nullptr;
        delete s_DistanceAtlas;    s_DistanceAtlas    = nullptr;
        delete s_TraceShader;      s_TraceShader      = nullptr;
        delete s_UpdateShader;     s_UpdateShader     = nullptr;
        delete s_BorderCopyShader; s_BorderCopyShader = nullptr;
        delete s_ClassifyShader;   s_ClassifyShader   = nullptr;
    }

    uint32_t DDGIPass::GetIrradianceAtlasID() {
        return s_IrradianceAtlas ? s_IrradianceAtlas->GetRendererID() : 0;
    }
    uint32_t DDGIPass::GetDistanceAtlasID() {
        return s_DistanceAtlas ? s_DistanceAtlas->GetRendererID() : 0;
    }
    Buffer* DDGIPass::GetProbeStateBuffer() {
        return s_ProbeStateBuffer;
    }

    glm::ivec3 DDGIPass::GetGridSize()    { return s_GridSize; }
    glm::vec3  DDGIPass::GetProbeOrigin()  { return s_ProbeOrigin; }
    glm::vec3  DDGIPass::GetProbeSpacing() { return s_ProbeSpacing; }
    int DDGIPass::GetProbesPerRow()        { return s_ProbesPerRow; }
}
