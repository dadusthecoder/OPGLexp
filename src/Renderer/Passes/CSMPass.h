#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <functional>
#include <vector>

namespace lgt {

    class CSMPass {
    public:
        static void Init(uint32_t resolution);
        static void Shutdown();
        static void Resize(uint32_t resolution);

        // renderCallback is used to render the scene into the depth map
        static void Execute(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::vec3& lightDir, float nearPlane, float farPlane, std::function<void(const glm::mat4& lightVP)> renderCallback);

        static uint32_t GetShadowMapArrayID();
        static const glm::mat4* GetLightSpaceMatrices();
        static const float* GetCascadeSplits();

    private:
        static void ComputeCascadeSplits(float nearPlane, float farPlane);
        static void ComputeLightSpaceMatrices(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::vec3& lightDir, float nearPlane, float farPlane);
        static std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& projMatrix, const glm::mat4& viewMatrix);
        static std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& projMatrix, const glm::mat4& viewMatrix, float nearZ, float farZ);

        static uint32_t s_Resolution;
        static uint32_t s_FBO;
        static uint32_t s_ShadowMapArray;
        static glm::mat4 s_LightSpaceMatrices[4];
        static float s_CascadeSplits[4];
    };

}
