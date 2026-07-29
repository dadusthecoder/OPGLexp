#pragma once

#include <vector>
#include <array>
#include <glm/glm.hpp>
#include "Buffer.h"
#include "Shader.h"
#include "Pipeline.h"
#include "Texture.h"

namespace lgt {

    class Material;
    class Mesh;

    struct RenderCommand {
        Mesh* mesh = nullptr;            // Preferred: Mesh owns VAO with layout
        Buffer* vertexBuffer = nullptr;  // Legacy fallback
        Buffer* indexBuffer = nullptr;   // Legacy fallback
        Material* material = nullptr;
        Pipeline* pipeline = nullptr;
        uint32_t indexCount = 0;
        uint32_t instanceCount = 1;
        glm::mat4 transform = glm::mat4(1.0f);
        
        // Sorting key (e.g. depth or material ID)
        uint64_t sortKey = 0;
        
        // Skeletal Animation
        const std::array<glm::mat4, 256>* skinMatrices = nullptr;
    };

    class RenderCommandQueue {
    public:
        RenderCommandQueue() = default;
        ~RenderCommandQueue() = default;

        void Submit(const RenderCommand& command);
        void Sort();
        void Clear();

        friend class Renderer;
    private:
        std::vector<RenderCommand> m_Commands;
    };

}
