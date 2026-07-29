#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "../Core/Shader.h"
#include "../Core/Buffer.h"

namespace lgt {
    class LightCullingPass {
    public:
        static void Init(uint32_t width, uint32_t height);
        static void Shutdown();
        static void Resize(uint32_t width, uint32_t height);

        static void Execute(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::mat4& invProjMatrix, uint32_t lightCount, float nearPlane, float farPlane);

        static Buffer* GetLightGridBuffer() { return s_LightGridBuffer; }
        static Buffer* GetLightIndexBuffer() { return s_LightIndexBuffer; }
        static glm::ivec3 GetGridSize() { return s_GridSize; }

    private:
        static void RecomputeAABBs(const glm::mat4& invProjMatrix, float nearPlane, float farPlane);

    private:
        static Shader* s_ClusterAABBShader;
        static Shader* s_ClusterCullShader;

        static Buffer* s_ClusterAABBBuffer;
        static Buffer* s_LightGridBuffer;
        static Buffer* s_LightIndexBuffer;
        static Buffer* s_GlobalIndexCountBuffer;

        static glm::ivec3 s_GridSize;
        static uint32_t s_Width;
        static uint32_t s_Height;
        static bool s_AABBsNeedUpdate;
    };
}
