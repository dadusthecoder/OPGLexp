#pragma once
#include <glm/glm.hpp>

namespace lgt {

    class DebugRenderer {
    public:
        static void Init();
        static void Shutdown();

        static void BeginScene(const glm::mat4& viewProjMatrix);
        static void EndScene();

        static void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color);

        struct LineVertex {
            glm::vec3 Position;
            glm::vec4 Color;
        };
    private:
    };

}
