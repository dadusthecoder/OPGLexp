#pragma once
#include "../Core/Texture.h"
#include "../Core/Shader.h"
#include "../Core/Framebuffer.h"
#include <string>
#include <cstdint>

namespace lgt {
    class IBLPass {
    public:
        static void BakeFromEquirect(const std::string& hdrPath);
        static bool IsReady();
        static void Shutdown();
        
        static uint32_t GetIrradianceMapID();
        static uint32_t GetPrefilterMapID();
        static uint32_t GetBrdfLutID();

    private:
        static Texture* s_EnvCubemap;
        static Texture* s_IrradianceMap;
        static Texture* s_PrefilterMap;
        static Texture* s_BrdfLut;
        static bool s_Ready;
        
        static void RenderToCubemap(Shader* shader, Texture* target, int size);
        static void BakeBrdfLut();
    };
}
