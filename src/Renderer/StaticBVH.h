#pragma once
#include "AccelerationStructure.h"
#include <glm/glm.hpp>
#include <vector>

namespace lgt {

struct BVHNodeGPU {
    float minX, minY, minZ;
    int   leftFirst;
    float maxX, maxY, maxZ;
    int   count;
};

class StaticBVH : public AccelerationStructure {
public:
    StaticBVH();
    ~StaticBVH() override;

    void Build(const std::vector<std::shared_ptr<SceneNode>>& roots) override;
    void UploadToGPU() override;
    void Bind() const override;
    void Unbind() const override;
    
    bool IsBuilt() const override { return m_IsBuilt; }
    int GetNodeCount() const override { return m_NodesUsed; }
    int GetTriangleCount() const override { return static_cast<int>(m_Indices.size() / 3); }
    GLuint GetNodesSSBO() const override { return m_NodesSSBO; }
    GLuint GetVerticesSSBO() const override { return m_VerticesSSBO; }
    GLuint GetIndicesSSBO() const override { return m_IndicesSSBO; }

private:
    struct BuildNode {
        glm::vec3 boundsMin = glm::vec3(1e30f);
        glm::vec3 boundsMax = glm::vec3(-1e30f);
        int leftFirst = 0;
        int count = 0;
    };

    struct BVHTriangle {
        glm::vec3 v0, v1, v2;
        glm::vec3 centroid;
    };

    struct Bin {
        int boundsCount = 0;
        glm::vec3 boundsMin = glm::vec3(1e30f);
        glm::vec3 boundsMax = glm::vec3(-1e30f);
    };

    void ExtractGeometry(const std::vector<std::shared_ptr<SceneNode>>& roots);
    void UpdateNodeBounds(int nodeIdx);
    void Subdivide(int nodeIdx);

    std::vector<BuildNode> m_Nodes;
    std::vector<BVHNodeGPU> m_GPUNodes;
    std::vector<glm::vec4> m_Vertices; // vec4 for std430 compatibility
    std::vector<uint32_t> m_Indices;
    
    std::vector<BVHTriangle> m_Triangles;
    std::vector<uint32_t> m_TriangleIndices;
    
    int m_NodesUsed = 0;
    bool m_IsBuilt = false;

    GLuint m_NodesSSBO = 0;
    GLuint m_VerticesSSBO = 0;
    GLuint m_IndicesSSBO = 0;
};

} // namespace lgt
