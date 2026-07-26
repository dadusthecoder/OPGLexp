#pragma once
#include <glm/glm.hpp>

namespace lgt {

    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoord;
    };

    struct Meshlet {
        uint32_t vertexOffset;
        uint32_t triangleOffset;
        uint32_t vertexCount;
        uint32_t triangleCount;

        // Bounding sphere for culling: xyz = center, w = radius
        glm::vec4 bounds;
    };

    // Standard OpenGL indirect draw command structure
    struct DrawCommand {
        uint32_t count;         // Number of indices
        uint32_t instanceCount; // Number of instances
        uint32_t firstIndex;    // Offset into index buffer
        uint32_t baseVertex;    // Added to each index
        uint32_t baseInstance;  // Offset into instance buffer
    };

    struct InstanceData {
        glm::mat4 Transform;
        uint32_t firstMeshlet;
        uint32_t meshletCount;
        uint32_t padding[2]; // Pad to 16 bytes alignment
    };

}
