#include "StaticBVH.h"
#include "Renderer/Scene.h"
#include "Renderer/renderer.h"
#include "Helpers/Logger.h"
#include <chrono>
#include <algorithm>

namespace lgt {

StaticBVH::StaticBVH() {
    glGenBuffers(1, &m_NodesSSBO);
    glGenBuffers(1, &m_VerticesSSBO);
    glGenBuffers(1, &m_IndicesSSBO);
}

StaticBVH::~StaticBVH() {
    glDeleteBuffers(1, &m_NodesSSBO);
    glDeleteBuffers(1, &m_VerticesSSBO);
    glDeleteBuffers(1, &m_IndicesSSBO);
}

void StaticBVH::ExtractGeometry(const std::vector<std::shared_ptr<SceneNode>>& roots) {
    m_Vertices.clear();
    m_Indices.clear();
    m_Triangles.clear();

    std::vector<const SceneNode*> stack;
    for (const auto& root : roots) {
        if (root) stack.push_back(root.get());
    }

    uint32_t indexOffset = 0;

    while (!stack.empty()) {
        const SceneNode* node = stack.back();
        stack.pop_back();

        for (const auto& child : node->children) {
            if (child) stack.push_back(child.get());
        }

        glm::mat4 globalTransform = node->globalTransform;

        for (const auto& mesh : node->meshes) {
            // Read back vertex and index data from VRAM
            glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
            int vboSize = 0;
            glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &vboSize);
            size_t vertexCount = vboSize / sizeof(Vertex);
            
            std::vector<Vertex> tempVertices(vertexCount);
            glGetBufferSubData(GL_ARRAY_BUFFER, 0, vboSize, tempVertices.data());

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);
            int iboSize = 0;
            glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &iboSize);
            std::vector<uint32_t> tempIndices(mesh.indexCount);
            glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, mesh.indexCount * sizeof(uint32_t), tempIndices.data());

            // Transform vertices into world space
            for (const auto& v : tempVertices) {
                glm::vec4 pos = globalTransform * glm::vec4(v.position, 1.0f);
                m_Vertices.push_back(glm::vec4(glm::vec3(pos) / pos.w, 0.0f));
            }

            // Extract triangles
            for (size_t i = 0; i < mesh.indexCount; i += 3) {
                uint32_t i0 = tempIndices[i] + indexOffset;
                uint32_t i1 = tempIndices[i+1] + indexOffset;
                uint32_t i2 = tempIndices[i+2] + indexOffset;

                m_Indices.push_back(i0);
                m_Indices.push_back(i1);
                m_Indices.push_back(i2);

                BVHTriangle tri;
                tri.v0 = glm::vec3(m_Vertices[i0]);
                tri.v1 = glm::vec3(m_Vertices[i1]);
                tri.v2 = glm::vec3(m_Vertices[i2]);
                tri.centroid = (tri.v0 + tri.v1 + tri.v2) * 0.333333f;
                m_Triangles.push_back(tri);
            }

            indexOffset += static_cast<uint32_t>(vertexCount);
        }
    }
}

void StaticBVH::UpdateNodeBounds(int nodeIdx) {
    BuildNode& node = m_Nodes[nodeIdx];
    node.boundsMin = glm::vec3(1e30f);
    node.boundsMax = glm::vec3(-1e30f);

    for (int i = 0; i < node.count; ++i) {
        uint32_t triIdx = m_TriangleIndices[node.leftFirst + i];
        const BVHTriangle& tri = m_Triangles[triIdx];
        node.boundsMin = glm::min(node.boundsMin, glm::min(tri.v0, glm::min(tri.v1, tri.v2)));
        node.boundsMax = glm::max(node.boundsMax, glm::max(tri.v0, glm::max(tri.v1, tri.v2)));
    }
}

void StaticBVH::Subdivide(int nodeIdx) {
    BuildNode& node = m_Nodes[nodeIdx];

    // Leaf condition: max 4 triangles per leaf
    if (node.count <= 4) return;

    // SAH Binned Construction
    const int BINS = 8;
    
    // Find bounds of centroids for building bins
    glm::vec3 centroidMin(1e30f), centroidMax(-1e30f);
    for (int i = 0; i < node.count; ++i) {
        uint32_t triIdx = m_TriangleIndices[node.leftFirst + i];
        const BVHTriangle& tri = m_Triangles[triIdx];
        centroidMin = glm::min(centroidMin, tri.centroid);
        centroidMax = glm::max(centroidMax, tri.centroid);
    }

    glm::vec3 extent = centroidMax - centroidMin;
    if (extent.x == 0.0f && extent.y == 0.0f && extent.z == 0.0f) return;

    int bestAxis = -1;
    float bestPos = 0;
    float bestCost = 1e30f;
    
    float nodeArea = 0.0f;
    glm::vec3 e = node.boundsMax - node.boundsMin;
    nodeArea = e.x * e.y + e.y * e.z + e.z * e.x;

    for (int axis = 0; axis < 3; ++axis) {
        if (centroidMax[axis] == centroidMin[axis]) continue;

        Bin bins[BINS];
        float scale = BINS / (centroidMax[axis] - centroidMin[axis]);

        for (int i = 0; i < node.count; ++i) {
            uint32_t triIdx = m_TriangleIndices[node.leftFirst + i];
            const BVHTriangle& tri = m_Triangles[triIdx];
            int binIdx = std::min(BINS - 1, static_cast<int>((tri.centroid[axis] - centroidMin[axis]) * scale));
            
            bins[binIdx].boundsCount++;
            bins[binIdx].boundsMin = glm::min(bins[binIdx].boundsMin, glm::min(tri.v0, glm::min(tri.v1, tri.v2)));
            bins[binIdx].boundsMax = glm::max(bins[binIdx].boundsMax, glm::max(tri.v0, glm::max(tri.v1, tri.v2)));
        }

        // Evaluate SAH for planes between bins
        float leftArea[BINS - 1], rightArea[BINS - 1];
        int leftCount[BINS - 1], rightCount[BINS - 1];
        
        glm::vec3 leftMin(1e30f), leftMax(-1e30f);
        int leftSum = 0;
        for (int i = 0; i < BINS - 1; ++i) {
            leftSum += bins[i].boundsCount;
            leftCount[i] = leftSum;
            leftMin = glm::min(leftMin, bins[i].boundsMin);
            leftMax = glm::max(leftMax, bins[i].boundsMax);
            glm::vec3 le = leftMax - leftMin;
            leftArea[i] = le.x * le.y + le.y * le.z + le.z * le.x;
        }

        glm::vec3 rightMin(1e30f), rightMax(-1e30f);
        int rightSum = 0;
        for (int i = BINS - 1; i > 0; --i) {
            rightSum += bins[i].boundsCount;
            rightCount[i - 1] = rightSum;
            rightMin = glm::min(rightMin, bins[i].boundsMin);
            rightMax = glm::max(rightMax, bins[i].boundsMax);
            glm::vec3 re = rightMax - rightMin;
            rightArea[i - 1] = re.x * re.y + re.y * re.z + re.z * re.x;
        }

        scale = (centroidMax[axis] - centroidMin[axis]) / BINS;
        for (int i = 0; i < BINS - 1; ++i) {
            float cost = leftCount[i] * leftArea[i] / nodeArea + rightCount[i] * rightArea[i] / nodeArea;
            if (cost < bestCost) {
                bestCost = cost;
                bestAxis = axis;
                bestPos = centroidMin[axis] + scale * (i + 1);
            }
        }
    }

    // SAH traversal step cost assumption, if cost to split > not splitting, terminate.
    // Assuming traversal cost = 1.0, intersection cost = 1.0 for simplicity.
    if (bestCost >= node.count) return;

    // Partition logic
    int i = node.leftFirst;
    int j = i + node.count - 1;
    while (i <= j) {
        uint32_t triIdx = m_TriangleIndices[i];
        const BVHTriangle& tri = m_Triangles[triIdx];
        if (tri.centroid[bestAxis] < bestPos) {
            i++;
        } else {
            std::swap(m_TriangleIndices[i], m_TriangleIndices[j--]);
        }
    }

    int leftCount = i - node.leftFirst;
    if (leftCount == 0 || leftCount == node.count) return;

    int leftChildIdx = m_NodesUsed++;
    int rightChildIdx = m_NodesUsed++;

    m_Nodes[leftChildIdx].leftFirst = node.leftFirst;
    m_Nodes[leftChildIdx].count = leftCount;
    UpdateNodeBounds(leftChildIdx);
    
    m_Nodes[rightChildIdx].leftFirst = i;
    m_Nodes[rightChildIdx].count = node.count - leftCount;
    UpdateNodeBounds(rightChildIdx);

    node.leftFirst = leftChildIdx;
    node.count = 0;

    Subdivide(leftChildIdx);
    Subdivide(rightChildIdx);
}

void StaticBVH::Build(const std::vector<std::shared_ptr<SceneNode>>& roots) {
    auto startTime = std::chrono::high_resolution_clock::now();

    ExtractGeometry(roots);

    if (m_Triangles.empty()) {
        m_IsBuilt = false;
        return;
    }

    m_TriangleIndices.resize(m_Triangles.size());
    for (size_t i = 0; i < m_Triangles.size(); ++i) {
        m_TriangleIndices[i] = static_cast<uint32_t>(i);
    }

    m_Nodes.resize(m_Triangles.size() * 2 - 1);
    m_NodesUsed = 1;

    m_Nodes[0].leftFirst = 0;
    m_Nodes[0].count = static_cast<int>(m_Triangles.size());
    UpdateNodeBounds(0);
    Subdivide(0);

    // Reorder indices arrays so leaf nodes can just use a contiguous range
    std::vector<uint32_t> reorderedIndices(m_Indices.size());
    for (size_t i = 0; i < m_TriangleIndices.size(); ++i) {
        uint32_t triIdx = m_TriangleIndices[i];
        reorderedIndices[i * 3 + 0] = m_Indices[triIdx * 3 + 0];
        reorderedIndices[i * 3 + 1] = m_Indices[triIdx * 3 + 1];
        reorderedIndices[i * 3 + 2] = m_Indices[triIdx * 3 + 2];
    }
    m_Indices = std::move(reorderedIndices);

    // Transform internal representation to GPU-aligned std430 struct
    m_GPUNodes.resize(m_NodesUsed);
    for (int i = 0; i < m_NodesUsed; ++i) {
        m_GPUNodes[i].minX = m_Nodes[i].boundsMin.x;
        m_GPUNodes[i].minY = m_Nodes[i].boundsMin.y;
        m_GPUNodes[i].minZ = m_Nodes[i].boundsMin.z;
        m_GPUNodes[i].maxX = m_Nodes[i].boundsMax.x;
        m_GPUNodes[i].maxY = m_Nodes[i].boundsMax.y;
        m_GPUNodes[i].maxZ = m_Nodes[i].boundsMax.z;
        m_GPUNodes[i].count = m_Nodes[i].count;
        
        if (m_Nodes[i].count > 0) {
            // Leaf node: leftFirst holds the index of the first triangle (not the vertex index)
            m_GPUNodes[i].leftFirst = m_Nodes[i].leftFirst;
        } else {
            // Inner node: leftFirst holds index of left child
            m_GPUNodes[i].leftFirst = m_Nodes[i].leftFirst;
        }
    }

    m_IsBuilt = true;

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    CORE_INFO("BVH built: {0} nodes, {1} triangles in {2} ms", m_NodesUsed, m_Triangles.size(), duration);
}

void StaticBVH::UploadToGPU() {
    if (!m_IsBuilt) return;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_NodesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_GPUNodes.size() * sizeof(BVHNodeGPU), m_GPUNodes.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_VerticesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_Vertices.size() * sizeof(glm::vec4), m_Vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_IndicesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_Indices.size() * sizeof(uint32_t), m_Indices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void StaticBVH::Bind() const {
    if (!m_IsBuilt) return;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_NodesSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_VerticesSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_IndicesSSBO);
}

void StaticBVH::Unbind() const {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, 0);
}

} // namespace lgt
