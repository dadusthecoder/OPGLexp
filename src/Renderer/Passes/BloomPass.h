#pragma once
#include "../Core/Framebuffer.h"
#include "../Core/Shader.h"
#include <cstdint>
#include <vector>

namespace lgt {
    class BloomPass {
    public:
        static void Init(uint32_t width, uint32_t height);
        static void Execute(uint32_t hdrTextureID, float threshold, float strength);
        static void Resize(uint32_t width, uint32_t height);
        static void Shutdown();
        static uint32_t GetBloomTextureID();

    private:
        static constexpr int NUM_MIPS = 6;
        static Shader* s_DownsampleShader;
        static Shader* s_UpsampleShader;
        static std::vector<Framebuffer*> s_MipFBOs;
        static uint32_t s_Width, s_Height;
        static uint32_t s_QuadVAO;
    };
}
