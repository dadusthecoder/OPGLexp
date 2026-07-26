#pragma once

#include <glm/glm.hpp>
#include "RenderCommandQueue.h"

namespace lgt {

    class Renderer {
    public:
        static void Init();
        static void Shutdown();

        static void BeginFrame();
        static void EndFrame();

        static void BeginScene(const glm::mat4& viewProjection, const glm::vec3& cameraPosition);

        static void OnWindowResize(uint32_t width, uint32_t height);
        static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
        static void SetClearColor(const glm::vec4& color);
        static void Clear();

        static void* GetFinalColorBufferTextureID();

        struct LightData {
            glm::vec3 Position;
            glm::vec3 Direction;
            glm::vec3 Color;
            float Intensity;
            int Type; // 0 = Directional, 1 = Point
            float Radius;
        };

        static void SubmitLight(const LightData& light);
        static void Submit(const RenderCommand& command);
        static void ExecuteQueue();

        static RenderCommandQueue& GetQueue();
    private:
        static RenderCommandQueue s_CommandQueue;
    };

}
