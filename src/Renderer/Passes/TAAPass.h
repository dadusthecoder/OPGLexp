#pragma once
#include "../Core/Framebuffer.h"
#include "../Core/Shader.h"
#include "../Core/Texture.h"
#include <cstdint>
#include <glm/glm.hpp>

namespace lgt {
    class TAAPass {
    public:
        static void Init(uint32_t width, uint32_t height);
        static glm::vec2 GetJitter(int frameIndex);
        static void Execute(uint32_t currentColorID, uint32_t velocityID, uint32_t depthID, int frameIndex);
        static void Resize(uint32_t width, uint32_t height);
        static void Shutdown();
        static uint32_t GetResolvedTextureID();

    private:
        static Shader*     s_TAAShader;
        static Framebuffer* s_HistoryFBOs[2];
        static int         s_CurrentHistory;
        static uint32_t    s_Width, s_Height;
        static uint32_t    s_QuadVAO;
    };
}
