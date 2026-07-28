#pragma once
#include <cstdint>
#include <glm/glm.hpp>

namespace lgt {

    class RTShadowPass {
    public:
        static void Init(uint32_t width, uint32_t height);
        static void Shutdown();

        static void Execute(uint32_t depthID, uint32_t normalID, const glm::mat4& invViewProj, const glm::vec3& cameraPos, const glm::vec3& lightDir);
        static void Resize(uint32_t width, uint32_t height);

        static uint32_t GetShadowMaskTextureID();

    private:
        static uint32_t s_Width;
        static uint32_t s_Height;
    };

}
