#pragma once
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

namespace lgt {

    struct BVHNode {
        glm::vec3 aabbMin;   // 12 bytes
        uint32_t  leftFirst; // 4 bytes — if isLeaf: first tri index; else: left child index
        glm::vec3 aabbMax;   // 12 bytes
        uint32_t  triCount;  // 4 bytes — 0 = internal node, >0 = leaf
    }; // 32 bytes

    struct BVHTriangle {
        glm::vec4 v0; // xyz=pos, w=unused
        glm::vec4 v1;
        glm::vec4 v2;
        glm::vec4 normal; // xyz=face normal, w=unused
    }; // 64 bytes

    class BVHBuilder {
    public:
        void Build(const std::vector<float>& vertices, const std::vector<uint32_t>& indices);
        
        const std::vector<BVHNode>& GetNodes() const { return m_Nodes; }
        const std::vector<BVHTriangle>& GetTriangles() const { return m_Triangles; }
        
    private:
        std::vector<BVHNode> m_Nodes;
        std::vector<BVHTriangle> m_Triangles;

        struct Bin {
            glm::vec3 aabbMin = glm::vec3(1e30f);
            glm::vec3 aabbMax = glm::vec3(-1e30f);
            uint32_t triCount = 0;
        };

        void UpdateNodeBounds(uint32_t nodeIdx);
        void Subdivide(uint32_t nodeIdx);
        float FindBestSplitPlane(BVHNode& node, int& axis, float& splitPos);
        float CalculateNodeCost(BVHNode& node);
    };
}
