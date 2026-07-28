#pragma once
#include "../Core/Framebuffer.h"
#include "../Core/Shader.h"
#include "../Core/Texture.h"
#include <cstdint>

namespace lgt {
    class RTAOPass {
    public:
        static void Init(uint32_t width, uint32_t height);
        static void Execute(uint32_t gDepthID, uint32_t gNormalID, float radius, int numRays, int frameIndex);
        static void Resize(uint32_t width, uint32_t height);
        static void Shutdown();
        static uint32_t GetAOTextureID();

    private:
        static Shader*   s_RTAOShader;
        static Shader*   s_DenoiseShader;
        static Texture*  s_RawAO;
        static Texture*  s_DenoisedAO;
        static Texture*  s_BlueNoiseTex;
        static uint32_t  s_Width, s_Height;
        
        static void CreateTextures();
        static void GenerateBlueNoise();
    };
}
