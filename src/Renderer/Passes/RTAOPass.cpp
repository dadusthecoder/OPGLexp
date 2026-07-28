#include "RTAOPass.h"
#include "../../Vendor/glad.h"
#include "BVHPass.h"
#include <random>

namespace lgt {

    Shader*   RTAOPass::s_RTAOShader = nullptr;
    Shader*   RTAOPass::s_DenoiseShader = nullptr;
    Texture*  RTAOPass::s_RawAO = nullptr;
    Texture*  RTAOPass::s_DenoisedAO = nullptr;
    Texture*  RTAOPass::s_BlueNoiseTex = nullptr;
    uint32_t  RTAOPass::s_Width = 0;
    uint32_t  RTAOPass::s_Height = 0;

    void RTAOPass::Init(uint32_t width, uint32_t height) {
        s_Width = width;
        s_Height = height;
        s_RTAOShader = Shader::CreateCompute("res/shaders/RTAO.comp");
        s_DenoiseShader = Shader::CreateCompute("res/shaders/rtao_denoise.comp");
        CreateTextures();
        GenerateBlueNoise();
    }

    void RTAOPass::CreateTextures() {
        if (s_RawAO) delete s_RawAO;
        if (s_DenoisedAO) delete s_DenoisedAO;

        TextureDescriptor desc;
        desc.width = s_Width;
        desc.height = s_Height;
        desc.format = TextureFormat::R8;
        desc.wrapS = TextureWrap::ClampToEdge;
        desc.wrapT = TextureWrap::ClampToEdge;
        desc.minFilter = TextureFilter::Nearest;
        desc.magFilter = TextureFilter::Nearest;
        desc.generateMipmaps = false;

        s_RawAO = Texture::Create(desc);
        s_DenoisedAO = Texture::Create(desc);
    }

    void RTAOPass::GenerateBlueNoise() {
        std::mt19937 rng(42);
        std::uniform_int_distribution<uint32_t> dist(0, 255);
        uint8_t noiseData[128 * 128];
        for (int i = 0; i < 128 * 128; i++) {
            noiseData[i] = (uint8_t)dist(rng);
        }

        TextureDescriptor desc;
        desc.width = 128;
        desc.height = 128;
        desc.format = TextureFormat::R8;
        desc.wrapS = TextureWrap::Repeat;
        desc.wrapT = TextureWrap::Repeat;
        desc.minFilter = TextureFilter::Nearest;
        desc.magFilter = TextureFilter::Nearest;
        desc.generateMipmaps = false;

        s_BlueNoiseTex = Texture::Create(desc);
        s_BlueNoiseTex->SetData(noiseData, sizeof(noiseData));
    }

    void RTAOPass::Execute(uint32_t gDepthID, uint32_t gNormalID, float radius, int numRays, int frameIndex) {
        if (!BVHPass::IsReady()) return;

        s_RTAOShader->Bind();
        s_RTAOShader->SetFloat("u_Radius", radius);
        s_RTAOShader->SetInt("u_NumRays", numRays);
        s_RTAOShader->SetInt("u_FrameIndex", frameIndex);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gDepthID);
        s_RTAOShader->SetInt("u_gDepth", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormalID);
        s_RTAOShader->SetInt("u_gNormal", 1);

        s_BlueNoiseTex->Bind(2);
        s_RTAOShader->SetInt("u_BlueNoise", 2);

        s_RawAO->BindImage(0, 0, false, 0, TextureAccess::WriteOnly);

        BVHPass::Bind(6, 7);

        glDispatchCompute((s_Width + 7) / 8, (s_Height + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        s_DenoiseShader->Bind();
        s_RawAO->BindImage(0, 0, false, 0, TextureAccess::ReadOnly);
        s_DenoisedAO->BindImage(1, 0, false, 0, TextureAccess::WriteOnly);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gNormalID);
        s_DenoiseShader->SetInt("u_gNormal", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gDepthID);
        s_DenoiseShader->SetInt("u_gDepth", 1);

        glDispatchCompute((s_Width + 7) / 8, (s_Height + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    void RTAOPass::Resize(uint32_t width, uint32_t height) {
        s_Width = width;
        s_Height = height;
        CreateTextures();
    }

    void RTAOPass::Shutdown() {
        delete s_RawAO; s_RawAO = nullptr;
        delete s_DenoisedAO; s_DenoisedAO = nullptr;
        delete s_BlueNoiseTex; s_BlueNoiseTex = nullptr;
        delete s_RTAOShader; s_RTAOShader = nullptr;
        delete s_DenoiseShader; s_DenoiseShader = nullptr;
    }

    uint32_t RTAOPass::GetAOTextureID() {
        return s_DenoisedAO ? s_DenoisedAO->GetRendererID() : 0;
    }
}
