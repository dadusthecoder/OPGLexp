#pragma once
#include "../Core/Texture.h"
#include "../Core/Shader.h"
#include "../Core/Buffer.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace lgt {
    class DDGIPass {
    public:
        static void Init(glm::ivec3 gridSize, glm::vec3 probeOrigin, glm::vec3 probeSpacing);
        static void Execute(glm::vec3 sunDir, glm::vec3 sunColor, float sunIntensity, int frameIndex);
        static void Resize(uint32_t width, uint32_t height);
        static void Shutdown();
        static uint32_t GetIrradianceAtlasID();
        
        static glm::ivec3 GetGridSize();
        static glm::vec3  GetProbeOrigin();
        static glm::vec3  GetProbeSpacing();

    private:
        static Shader*  s_TraceShader;
        static Shader*  s_UpdateShader;
        static Buffer*  s_RayDataBuffer;
        static Texture* s_IrradianceAtlas;
        
        static glm::ivec3 s_GridSize;
        static glm::vec3  s_ProbeOrigin;
        static glm::vec3  s_ProbeSpacing;
        static int s_TotalProbes;
        static int s_RaysPerProbe;
    };
}
